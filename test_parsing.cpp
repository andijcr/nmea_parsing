#include "catch2/catch.hpp"
#include "gps_parser.hpp"

TEST_CASE() {
	REQUIRE(to_GpsTrack("") == std::nullopt);
	REQUIRE(to_GpsTrack("$GNVTG,0.00,T,,M,0.00,N,0.00,K,N*2C") != std::nullopt);
	auto trak = *to_GpsTrack("$GNVTG,0.00,T,,M,0.00,N,0.00,K,N*2C");
	REQUIRE(trak.heading_true.has_value());
	REQUIRE(trak.heading_true.value() == milli_dec(0));
	REQUIRE_FALSE(trak.heading_magnetic.has_value());
	REQUIRE(trak.ground_speed_kmh.has_value());
	REQUIRE(trak.ground_speed_kmh.value() == milli_dec(0));
}

TEST_CASE() {
	REQUIRE_FALSE(to_GpsFix("").has_value());
	REQUIRE(
		to_GpsFix("$GNGGA,124547.726,4124.8963,N,08151.6838,W,1,0,,,M,,M,,*54")
			.has_value());
	auto fix =
		to_GpsFix("$GNGGA,124547.726,4124.8963,N,08151.6838,W,1,0,,,M,,M,,*54")
			.value();
	REQUIRE(fix.coord.has_value());
	REQUIRE(fix.coord->lat.deg == 41);
	REQUIRE(fix.coord->lat.min == 24);
	REQUIRE(fix.coord->lat.decimal == micro_dec(896300));
	REQUIRE(fix.coord->lat_d == lat_dir::N);
}
