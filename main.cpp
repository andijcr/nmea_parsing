#include "fmt/printf.h"
#include "fmt/ranges.h"
#include "gps_parser.hpp"
#include "magic_enum.hpp"
#include "spdlog/spdlog.h"
#include <array>

#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

struct null_parse {
	auto parse(fmt::format_parse_context &ctx) {
		return std::find(ctx.begin(), ctx.end(), '}');
	}
};

template <> struct fmt::formatter<milli_dec> : public null_parse {
	template <typename FormatContext>
	auto format(milli_dec val, FormatContext &ctx) {
		int integral = int(val) / 1000;
		int decimal = std::abs(int(val) - integral * 1000);
		return format_to(ctx.out(), "{}.{:0>3}", integral, decimal);
	}
};
template <typename T>
struct fmt::formatter<std::optional<T>> : public null_parse {
	template <typename FormatContext>
	auto format(const std::optional<T> &val, FormatContext &ctx) {
		if (val) {
			return format_to(ctx.out(), "(opt: {})", *val);
		}
		return format_to(ctx.out(), "(opt empty)");
	}
};

template <> struct fmt::formatter<GpsQuality> : public null_parse {
	template <typename FormatContext>
	auto format(const GpsQuality &gq, FormatContext &ctx) {
		return format_to(
			ctx.out(), "gps_quality, fix: {}, pdop: {}, hdop: {}, vdop: {}",
			magic_enum::enum_name(gq.fix), gq.pdop, gq.hdop, gq.vdop);
	}
};

template <> struct fmt::formatter<GpsTrack> : public null_parse {
	template <typename FormatContext>
	auto format(const GpsTrack &gt, FormatContext &ctx) {
		return format_to(
			ctx.out(),
			"gps_track, heading_true: {}, heading_magnetic: {}, speed_kmh: {}",
			gt.heading_true, gt.heading_magnetic, gt.ground_speed_kmh);
	}
};

template <> struct fmt::formatter<DMM> : public null_parse {
	template <typename FormatContext>
	auto format(const DMM &d, FormatContext &ctx) {
		return format_to(ctx.out(), "{}°{}'.{:0>6}", d.deg, d.min,
						 int(d.decimal));
	}
};

template <> struct fmt::formatter<Pos> : public null_parse {
	template <typename FormatContext>
	auto format(const Pos &p, FormatContext &ctx) {
		return format_to(ctx.out(), "lat: {}{}, lon: {}{}", p.lat,
						 magic_enum::enum_name(p.lat_d), p.lon,
						 magic_enum::enum_name(p.lon_d));
	}
};

template <> struct fmt::formatter<GpsFix> : public null_parse {
	template <typename FormatContext>
	auto format(const GpsFix &gf, FormatContext &ctx) {
		return format_to(ctx.out(),
						 "gps_fix, ms_of_day: {}, coord: {}, fix: {}, sats: "
						 "{}, hdop: {}, altitude: {}mm, geoid_sep: {}mm",
						 gf.millisecs_of_day, gf.coord,
						 magic_enum::enum_name(gf.fix), gf.num_satellites_used,
						 gf.hdop, gf.altitude_mm, gf.geoid_sep_mm);
	}
};

constexpr auto gsa_sentences = std::array{
	"$GPGSA,A,1,,,,,,,,,,,,,,,*1E",
	"$GPGSA,A,2,,,,,,,,,,,,1.1,2.4,0.4,*1E",
	"$GPGSA,M,2,,,,,,,,,,,,1.1,2.4,0,*1E",
	"$GPGSA,M,2,,,,,,,,,,,,1,2932.004,0.0001,*1E",
	"$GPGSA,M,2,,,,,,,,,,,,1,-2932.004,1.0,*1E",
};

constexpr auto gtv_sentences =
	std::array{"$GNVTG,0.00,T,,M,0.00,N,0.00,K,N*2C"};

constexpr auto gga_sentences = std::array{
	"$GNGGA,124547.726,,,,,0,0,,,M,,M,,*54",
	"$GNGGA,124547.726,4124.8963,N,08151.6838,W,1,0,,,M,,M,,*54",
};
void benchmark() {
	ankerl::nanobench::Bench().run("range_version", []() {
		for (auto s : gsa_sentences) {
			// fmt::print("{}", to_GpsQuality(s));
			ankerl::nanobench::doNotOptimizeAway(to_GpsQuality(s));
		}
	});
}

int main() {
	spdlog::set_level(spdlog::level::trace);

	for (auto s : gsa_sentences) {
		fmt::print("phrase:{} range_v:{}\n", s, to_GpsQuality(s));
	}

	for (auto s : gtv_sentences) {
		fmt::print("phrase:{} range_v:{}\n", s, to_GpsTrack(s));
	}

	for (auto s : gga_sentences) {
		fmt::print("phrase:{} range_v:{}\n", s, to_GpsFix(s));
	}

	spdlog::set_level(spdlog::level::info);
	benchmark();
}
