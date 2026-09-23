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
    REQUIRE(dumpfloppy::container_from_path("foo.d71") ==
            dumpfloppy::container_kind::d71_c64);
    REQUIRE(dumpfloppy::container_from_path("bar.D81") ==
            dumpfloppy::container_kind::d81_c64);
    REQUIRE(dumpfloppy::container_from_path("work.ADF") ==
            dumpfloppy::container_kind::adf_amiga);
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

TEST_CASE("D71 and ADF sizes are not IBM trailers", "[geometry][cbm][adf]")
{
    const auto d71 = dumpfloppy::geometry_from_size(349696);
    REQUIRE(d71.bytes_per_sector == 256);
    REQUIRE(d71.media_name.find("1571") != std::string::npos);
    REQUIRE(d71.media_name.find("320K") == std::string::npos);

    const auto d71err = dumpfloppy::geometry_from_size(351062);
    REQUIRE(d71err.media_name.find("error map") != std::string::npos);

    /* 819200 is IBM 800K by size; analyse overrides when CBMFS is present. */
    const auto ibm800 = dumpfloppy::geometry_from_size(819200);
    REQUIRE(ibm800.bytes_per_sector == 512);
    REQUIRE(ibm800.media_name.find("800K") != std::string::npos);

    const auto d81err = dumpfloppy::geometry_from_size(822400);
    REQUIRE(d81err.bytes_per_sector == 256);
    REQUIRE(d81err.media_name.find("1581") != std::string::npos);

    const auto adf_dd = dumpfloppy::geometry_from_size(901120);
    REQUIRE(adf_dd.cylinders == 80);
    REQUIRE(adf_dd.heads == 2);
    REQUIRE(adf_dd.sectors_per_track == 11);
    REQUIRE(adf_dd.bytes_per_sector == 512);
    REQUIRE(adf_dd.media_name.find("Amiga") != std::string::npos);

    const auto adf_hd = dumpfloppy::geometry_from_size(1802240);
    REQUIRE(adf_hd.sectors_per_track == 22);
    REQUIRE(adf_hd.media_name.find("HD") != std::string::npos);
    REQUIRE(adf_hd.media_name.find("DMF") == std::string::npos);
}

TEST_CASE("media descriptor names", "[geometry]")
{
    REQUIRE(dumpfloppy::media_descriptor_name(0xF9).find("F9") != std::string::npos);
    REQUIRE(dumpfloppy::media_descriptor_name(0xF0).find("F0") != std::string::npos);
    REQUIRE(dumpfloppy::media_descriptor_name(0xFD).find("360") != std::string::npos);
}
