/**
 * @file test_d71.cpp
 * @brief Product wiring: analyse / listing / extract / refuse -u on D71 CBMFS.
 */
#include "d71_builder.hpp"
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/cbm.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/update.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace
{

dumpfloppy::floppy_image make_d71_image()
{
    dumpfloppy::floppy_image img{};
    img.bytes = dumpfloppy_test::make_sample_d71();
    img.path = "sample.d71";
    img.container = dumpfloppy::container_kind::d71_c64;
    return img;
}

} /* namespace */

TEST_CASE("analyse synthetic D71 skips FAT and fills CBM", "[d71][analyse]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(make_d71_image());
    REQUIRE(a.cbm.present);
    REQUIRE_FALSE(a.amiga.present);
    REQUIRE(a.cbm.media == dumpfloppy::cbm_media::d71);
    REQUIRE(a.cbm.media_name == "D71");
    REQUIRE(a.cbm.disk_name == "TEST 1571");
    REQUIRE(a.cbm.disk_id == "71");
    REQUIRE(a.cbm.entries.size() == 1u);
    REQUIRE(a.cbm.entries[0].name == "SIDE1");
    REQUIRE(a.cbm.entries[0].kind == dumpfloppy::cbm_file_kind::prg);
    REQUIRE(a.cbm.entries[0].first_track == 36);
    REQUIRE_FALSE(a.bpb.looks_valid);
    REQUIRE(a.kind == dumpfloppy::fat_kind::unknown);
    REQUIRE(a.entries.empty());
    REQUIRE_FALSE(a.flux.present);
    REQUIRE(a.image.size_geometry.media_name.find("1571") != std::string::npos);
    REQUIRE(a.image.size_geometry.bytes_per_sector == 256);
}

TEST_CASE("write_report lists D71 CBMFS not FAT", "[d71][report]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_d71_image());
    dumpfloppy::report_options opt{};
    opt.color = false;
    opt.hex_boot = true;
    std::ostringstream plain;
    dumpfloppy::write_report(a, plain, opt);
    const std::string s = plain.str();
    REQUIRE(s.find("C64 D71 / CBMFS") != std::string::npos);
    REQUIRE(s.find("TEST 1571") != std::string::npos);
    REQUIRE(s.find("SIDE1") != std::string::npos);
    REQUIRE(s.find("PRG") != std::string::npos);
    REQUIRE(s.find("BIOS PARAMETER BLOCK") == std::string::npos);
    REQUIRE(s.find("BOOT SECTOR HEX") == std::string::npos);
    REQUIRE(s.find("FAT12") == std::string::npos);
}

TEST_CASE("extract writes D71 PRG from track 36", "[d71][extract]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_d71_image());
    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "d71-out";
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);

    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 1);
    REQUIRE(err.str().empty());

    std::ifstream prg(dest / "SIDE1.prg", std::ios::binary);
    REQUIRE(prg);
    const std::vector<uint8_t> body((std::istreambuf_iterator<char>(prg)),
                                    std::istreambuf_iterator<char>());
    REQUIRE(body == dumpfloppy_test::sample_d71_prg_bytes());
}

TEST_CASE("update same-size D71 PRG in place", "[d71][update]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(make_d71_image());
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto host = dir / "SIDE1.prg";
    std::vector<uint8_t> neu = dumpfloppy_test::sample_d71_prg_bytes();
    std::fill(neu.begin(), neu.end(), static_cast<uint8_t>(0x71));
    {
        std::ofstream out(host, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out.write(reinterpret_cast<const char*>(neu.data()),
                  static_cast<std::streamsize>(neu.size()));
    }
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.push_back(host);
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == 1);
    REQUIRE(err.str().empty());
    REQUIRE(dumpfloppy::read_cbm_file(a.image.bytes, a.cbm.media, a.cbm.entries[0]) ==
            neu);
}

TEST_CASE("load_image .d71 is container d71_c64", "[d71][image]")
{
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto path = dir / "sample.d71";
    const auto bytes = dumpfloppy_test::make_sample_d71();
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded);
    REQUIRE(loaded->container == dumpfloppy::container_kind::d71_c64);
    REQUIRE(loaded->size_geometry.media_name.find("1571") != std::string::npos);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.cbm.present);
    REQUIRE(a.cbm.media == dumpfloppy::cbm_media::d71);
    REQUIRE(a.cbm.disk_name == "TEST 1571");
}
