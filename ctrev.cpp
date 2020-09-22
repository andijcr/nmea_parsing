#include "ctre.hpp"
#include "range/v3/view.hpp"
#include "sentences.hpp"
#include <charconv>

constexpr auto rng_to_string_view = [](auto &&rng) -> std::string_view {
	return std::string_view(&*rng.begin(), ranges::distance(rng));
};

namespace views = ranges::views;

constexpr auto to_dilution2(std::string_view tok) -> float {
	auto i_parts = tok | views::split('.') |
				   views::transform(rng_to_string_view) |
				   views::transform([](std::string_view p) {
					   int val{};
					   std::from_chars(p.begin(), p.end(), val);
					   return std::pair{val, p.size()};
				   });

	auto it_parts = i_parts.begin();
	const auto [integral, _] = *it_parts++;
	const auto [decimal, shift] = *it_parts;
	if (decimal == 0) {
		return integral;
	}
	return integral + decimal / (10.f * shift);
}
constexpr auto to_dilution(std::string_view tok) -> std::optional<float> {
	if (tok.empty()) {
		return std::nullopt;
	}
	auto [re, i, d] = ctre::match<R"p(([0-9]+)\.?([0-9]+)?)p">(tok);
	if (!re) {
		return std::nullopt;
	}
	auto integral = i.to_view();
	int i_val{};
	std::from_chars(integral.begin(), integral.end(), i_val);

	auto decimal = d.to_view();
	int d_val{};
	std::from_chars(decimal.begin(), decimal.end(), d_val);

	return i_val + d_val / 10.f;
};

using namespace std::literals;
auto to_GpsQualityCTRE(std::string_view candidate)
	-> std::optional<GpsQuality> {
	auto [re, fix, pdop, hdop, vdop] = ctre::match<
		R"p(\$GPGSA,.,([123]),.*,.*,.*,.*,.*,.*,.*,.*,.*,.*,.*,([0-9\.]+)?,([0-9\.]+)?,([0-9\.]+)?,.*)p">(
		candidate);
	if (!re) {
		return std::nullopt;
	}

	constexpr auto fix_t = [](std::string_view tn) {
		if (tn == "2"sv) {
			return fix_type::f2D;
		}
		if (tn == "3"sv) {
			return fix_type::f3D;
		}
		return fix_type::none;
	};

	return GpsQuality{
		.fix = fix_t(fix),
		.pdop = to_dilution2(pdop),
		.hdop = to_dilution2(hdop),
		.vdop = to_dilution2(vdop),
	};
}
