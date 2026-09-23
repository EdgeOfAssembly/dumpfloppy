/**
 * @file test_geometry.cpp
 * @brief Standard floppy sizes and .ima vs .img container guess.
 */
#include "dumpfloppy/geometry.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

TEST_CASE("720K and 1.44M sizes map to CHS", "[geometry]")
{
    const auto k720 = dumpfloppy::geometry_from_size(737280);
    REQUIRE(k720.cylinders == 80);
    REQUIRE(k720.heads == 2);
    REQUIRE(k720.sectors_per_track == 9);
    REQUIRE(k720.media_name.find("720K") != std::string::npos);

    const auto k144 = dumpfloppy::geometry_from_size(1474560);
    REQUIRE(k144.sectors_per_track == 18);
    REQUIRE(k144.media_name.find("1.44") != std::string::npos);

    const auto k360 = dumpfloppy::geometry_from_size(368640);
    REQUIRE(k360.cylinders == 40);
    REQUIRE(k360.sectors_per_track == 9);
}

TEST_CASE("container_from_path treats .ima as WinImage", "[geometry]")
{
    REQUIRE(dumpfloppy::container_from_path("foo.IMA") ==
            dumpfloppy::container_kind::ima_winimage);
    REQUIRE(dumpfloppy::container_from_path("foo.img") ==
            dumpfloppy::container_kind::img_raw);
    REQUIRE(dumpfloppy::container_from_path("foo.D64") ==
            dumpfloppy::container_kind::d64_c64);
    REQUIRE(dumpfloppy::container_from_path("foo.bin") ==
            dumpfloppy::container_kind::unknown_raw);
}

TEST_CASE("D64 sizes are 1541, not 160K IBM trailer", "[geometry][d64]")
{
    const auto d64 = dumpfloppy::geometry_from_size(174848);
    REQUIRE(d64.cylinders == 0);
    REQUIRE(d64.bytes_per_sector == 256);
    REQUIRE(d64.media_name.find("1541") != std::string::npos);
    REQUIRE(d64.media_name.find("160K") == std::string::npos);

    const auto errmap = dumpfloppy::geometry_from_size(175531);
    REQUIRE(errmap.media_name.find("error map") != std::string::npos);
    REQUIRE(errmap.bytes_per_sector == 256);
}

TEST_CASE("media descriptor names", "[geometry]")
{
    REQUIRE(dumpfloppy::media_descriptor_name(0xF9).find("F9") != std::string::npos);
    REQUIRE(dumpfloppy::media_descriptor_name(0xF0).find("F0") != std::string::npos);
    REQUIRE(dumpfloppy::media_descriptor_name(0xFD).find("360") != std::string::npos);
}
