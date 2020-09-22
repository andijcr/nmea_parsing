#include "gps_parser.hpp"
#include "magic_enum.hpp"
#include "range/v3/algorithm.hpp"
#include "range/v3/view.hpp"

#include <charconv>

namespace views = ranges::views;
using namespace std::literals;

constexpr auto is_num(char c) { return c >= '0' && c <= '9'; }

constexpr auto rng_to_string_view = [](auto &&rng) -> std::string_view {
	return std::string_view(&*rng.begin(), ranges::distance(rng));
};

template <typename Rng> static auto get_num(Rng &&sv) {
	int val{};
	std::from_chars(sv.begin(), sv.end(), val);
	return val;
}
constexpr auto is_a_float_or_empty(std::string_view sv) {
	if (sv.size() == 0) {
		return true;
	}

	if (ranges::count(sv, '.') > 1) {
		return false;
	}

	if (ranges::count(sv, '-') > 1) {
		return false;
	}

	if (sv.starts_with('-')) {
		sv = sv.substr(1);
	}
	return ranges::all_of(sv, [](auto c) { return c == '.' || is_num(c); });
}

template <size_t POW> static auto to_scaled_int(std::string_view tok) -> int {
	auto sep = tok.find('.');

	// poor man constexpr std::pow
	constexpr auto scale = [] {
		int sc = 1;
		for (auto i = 0u; i < POW; ++i) {
			sc *= 10;
		}
		return sc;
	}();
	if (sep == tok.npos) {
		return get_num(tok) * scale;
	}

	auto int_range = tok.substr(0, sep);
	int integral = get_num(int_range);

	auto dec_tok = tok.substr(sep + 1);
	auto dec_range = views::concat(dec_tok, views::repeat('0'));
	std::array<char, POW> dec_part{};
	std::copy_n(dec_range.begin(), dec_part.size(), dec_part.begin());
	int decimal = get_num(dec_part);

	return integral * scale + (integral >= 0 ? decimal : -decimal);
}
static auto to_milli_dec(std::string_view tok) -> milli_dec {
	return milli_dec{to_scaled_int<3>(tok)};
}

static auto to_micro_dec(std::string_view tok) -> micro_dec {
	return micro_dec{to_scaled_int<6>(tok)};
}

constexpr auto to_opt_float = [](auto dilu) -> std::optional<milli_dec> {
	if (dilu.size() == 0)
		return std::nullopt;
	return to_milli_dec(dilu);
};
using namespace std::literals;
auto to_GpsQuality(std::string_view candidate) -> std::optional<GpsQuality> {
	if (!candidate.starts_with(GpsQuality::start)) {
		return std::nullopt;
	}

	auto tokens = candidate | views::split(',') | views::drop(2) |
				  views::transform(rng_to_string_view);

	if (ranges::empty(tokens)) {
		return std::nullopt;
	}

	auto tok_it = tokens.begin();
	const auto fix_t = [](std::string_view tn) {
		if (tn == "2"sv) {
			return fix_type::f2D;
		}
		if (tn == "3"sv) {
			return fix_type::f3D;
		}
		return fix_type::none;
	}(*tok_it++);

	if (fix_t == fix_type::none) {
		return GpsQuality{.fix = fix_type::none,
						  .pdop = std::nullopt,
						  .hdop = std::nullopt,
						  .vdop = std::nullopt};
	}

	auto dops = tokens | views::drop(GpsQuality::sats_len) | views::take(3);

	if (!(ranges::distance(dops) == 3 &&
		  ranges::all_of(dops, is_a_float_or_empty))) {
		return std::nullopt;
	}

	auto parsed_values = dops | views::transform(to_opt_float);

	auto p_it = parsed_values.begin();
	return GpsQuality{
		.fix = fix_t,
		.pdop = *p_it++,
		.hdop = *p_it++,
		.vdop = *p_it,
	};
}

auto to_GpsTrack(std::string_view candidate) -> std::optional<GpsTrack> {
	if (!candidate.starts_with(GpsTrack::start)) {
		return std::nullopt;
	}

	auto tokens = candidate | views::split(',') | views::drop(1) |
				  views::stride(2) | views::transform(rng_to_string_view);
	//	auto tokens =
	//		views::enumerate(candidate | views::split(',') | views::drop(1)) |
	//		views::for_each([](auto idx_v) {
	//			auto [idx, v] = idx_v;
	//			return ranges::yield_if(idx % 2 == 0, rng_to_string_view(v));
	//		});

	if (!ranges::all_of(tokens | views::take(3), is_a_float_or_empty)) {
		return std::nullopt;
	}

	auto parsed_values = tokens | views::transform(to_opt_float);

	auto p_it = parsed_values.begin();
	return GpsTrack{
		.heading_true = *p_it++,
		.heading_magnetic = *p_it++,
		.ground_speed_kmh = *p_it,
	};
}

static auto get_millisecs(std::string_view time) -> milli_dec {
	return milli_dec{(get_num(time.substr(0, 2)) * 60 * 60 +
					  get_num(time.substr(2, 2)) * 60 +
					  get_num(time.substr(4, 2))) *
						 1000 +
					 (time.size() > 6 ? to_scaled_int<3>(time.substr(6)) : 0)};
}

static auto is_Pos_or_empty(std::string_view lat, std::string_view lat_d,
							std::string_view lon, std::string_view lon_d)
	-> bool {

	// all are empty
	if (ranges::all_of(std::array{lat, lat_d, lon, lon_d},
					   [](auto e) { return e.size() == 0; })) {
		return true;
	}

	// or none is
	if (ranges::any_of(std::array{lat, lat_d, lon, lon_d},
					   [](auto e) { return e.size() == 0; })) {
		return false;
	}

	if (!(magic_enum::enum_contains<lat_dir>(lat_d) &&
		  magic_enum::enum_contains<lon_dir>(lon_d))) {
		return false;
	}

	if (!(is_a_float_or_empty(lat) && is_a_float_or_empty(lon))) {
		return false;
	}
	return true;
}

static auto getDMM(std::string_view data) -> DMM {

	auto sep = data.find('.');
	return {.deg =
				uint8_t(std::clamp(get_num(data.substr(0, sep - 2)), 0, 180)),
			.min = uint8_t(std::clamp(get_num(data.substr(sep - 2, 2)), 0, 60)),
			.decimal = to_micro_dec(data.substr(sep))};
}
static auto get_Pos_or_empty(std::string_view lat, std::string_view lat_d,
							 std::string_view lon, std::string_view lon_d)
	-> std::optional<Pos> {
	if (ranges::all_of(std::array{lat, lat_d, lon, lon_d},
					   [](auto e) { return e.size() == 0; })) {
		return std::nullopt;
	}

	return Pos{
		.lat = getDMM(lat),
		.lon = getDMM(lon),
		.lat_d = magic_enum::enum_cast<lat_dir>(lat_d).value_or(lat_dir::N),
		.lon_d = magic_enum::enum_cast<lon_dir>(lon_d).value_or(lon_dir::W),
	};
}

auto to_GpsFix(std::string_view candidate) -> std::optional<GpsFix> {
	if (!candidate.starts_with(GpsFix::start)) {
		return std::nullopt;
	}

	//"[$GNGGA,124547.726,,,,,0,0,,,M,,M,,*54]";
	if (!(ranges::count(candidate, ',') == 14 && candidate.back() != ',')) {
		return std::nullopt;
	}
	auto tokens = candidate | views::split(',') | views::drop(1) |
				  views::take(12) | views::transform(rng_to_string_view);
	auto tok_it = tokens.begin();

	auto time_fix = *tok_it++;
	if (!is_a_float_or_empty(time_fix)) {
		return std::nullopt;
	}

	const auto lat = *tok_it++;
	const auto lat_d = *tok_it++;
	const auto lon = *tok_it++;
	const auto lon_d = *tok_it++;
	if (!is_Pos_or_empty(lat, lat_d, lon, lon_d)) {
		return std::nullopt;
	}

	const auto fix_t = *tok_it++;
	if (!ranges::all_of(fix_t, is_num) && fix_t.size() == 1) {
		return std::nullopt;
	}

	const auto num_satellites = *tok_it++;
	if (!ranges::all_of(num_satellites, is_num)) {
		return std::nullopt;
	}

	const auto hdop = *tok_it++;
	if (!is_a_float_or_empty(hdop)) {
		return std::nullopt;
	}
	const auto altitude = *tok_it++;
	if (!is_a_float_or_empty(altitude)) {
		return std::nullopt;
	}
	if (*tok_it++ != "M"sv) {
		return std::nullopt;
	}

	const auto geoid_sep = *tok_it;
	if (!is_a_float_or_empty(geoid_sep)) {
		return std::nullopt;
	}

	return GpsFix{
		.millisecs_of_day = get_millisecs(time_fix),
		.coord = get_Pos_or_empty(lat, lat_d, lon, lon_d),
		.fix = [&] { return fix_system{fix_t[0] - '0'}; }(),
		.num_satellites_used = uint8_t(std::clamp<int>(
			get_num(num_satellites), 0, std::numeric_limits<uint8_t>::max())),
		.hdop = to_opt_float(hdop),
		.altitude_mm = to_opt_float(altitude),
		.geoid_sep_mm = to_opt_float(geoid_sep),
	};
}
