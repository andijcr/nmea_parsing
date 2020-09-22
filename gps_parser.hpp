#pragma once

#include <optional>
#include <string_view>

enum class fix_type { none = 1, f2D = 2, f3D = 3 };

enum class milli_dec : int32_t {};
struct GpsQuality {
	constexpr static auto start = std::string_view{"$GPGSA"};
	constexpr static auto sats_len = 12;
	fix_type fix;
	std::optional<milli_dec> pdop;
	std::optional<milli_dec> hdop;
	std::optional<milli_dec> vdop;
};

struct GpsTrack {
	constexpr static auto start = std::string_view{"$GNVTG"};
	std::optional<milli_dec> heading_true;
	std::optional<milli_dec> heading_magnetic;
	std::optional<milli_dec> ground_speed_kmh;
};

enum class micro_dec : int32_t {};
enum class fix_system { none = 0, gps = 1, dgps = 2 };
enum class lat_dir : uint8_t { N, S };
enum class lon_dir : uint8_t { W, E };
struct DMM {
	uint8_t deg;
	uint8_t min;
	micro_dec decimal;
};

struct Pos {
	DMM lat;
	DMM lon;
	lat_dir lat_d;
	lon_dir lon_d;
};

struct GpsFix {
	constexpr static auto start = std::string_view{"$GNGGA"};
	milli_dec millisecs_of_day;
	std::optional<Pos> coord;
	fix_system fix;
	uint8_t num_satellites_used;
	std::optional<milli_dec> hdop;
	std::optional<milli_dec> altitude_mm;
	std::optional<milli_dec> geoid_sep_mm;
};

auto to_GpsQuality(std::string_view candidate) -> std::optional<GpsQuality>;
auto to_GpsTrack(std::string_view candidate) -> std::optional<GpsTrack>;
auto to_GpsFix(std::string_view candidate) -> std::optional<GpsFix>;
