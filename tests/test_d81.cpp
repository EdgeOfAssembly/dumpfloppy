/**
 * @file test_d81.cpp
 * @brief Product wiring: analyse / listing / extract / refuse -u on D81 CBMFS.
 */
#include "d81_builder.hpp"
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/cbm.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/update.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace
{

dumpfloppy::floppy_image make_d81_image()
{
    dumpfloppy::floppy_image img{};
    img.bytes = dumpfloppy_test::make_sample_d81();
    img.path = "sample.d81";
    img.container = dumpfloppy::container_kind::d81_c64;
    return img;
}

} /* namespace */

TEST_CASE("analyse synthetic D81 skips FAT and fills CBM", "[d81][analyse]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(make_d81_image());
    REQUIRE(a.cbm.present);
    REQUIRE_FALSE(a.amiga.present);
    REQUIRE(a.cbm.media == dumpfloppy::cbm_media::d81);
    REQUIRE(a.cbm.media_name == "D81");
    REQUIRE(a.cbm.disk_name == "TEST 1581");
    REQUIRE(a.cbm.disk_id == "81");
    REQUIRE(a.cbm.dos_version == static_cast<uint8_t>('D'));
    REQUIRE(a.cbm.entries.size() == 1u);
    REQUIRE(a.cbm.entries[0].name == "HELLO81");
    REQUIRE(a.cbm.entries[0].kind == dumpfloppy::cbm_file_kind::prg);
    REQUIRE_FALSE(a.bpb.looks_valid);
    REQUIRE(a.kind == dumpfloppy::fat_kind::unknown);
    REQUIRE(a.entries.empty());
    REQUIRE_FALSE(a.flux.present);
    REQUIRE(a.image.size_geometry.media_name.find("1581") != std::string::npos);
    REQUIRE(a.image.size_geometry.cylinders == 80);
    REQUIRE(a.image.size_geometry.sectors_per_track == 40);
    REQUIRE(a.image.size_geometry.bytes_per_sector == 256);
}

TEST_CASE("write_report lists D81 CBMFS not FAT", "[d81][report]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_d81_image());
    dumpfloppy::report_options opt{};
    opt.color = false;
    opt.hex_boot = true;
    std::ostringstream plain;
    dumpfloppy::write_report(a, plain, opt);
    const std::string s = plain.str();
    REQUIRE(s.find("C64 D81 / CBMFS") != std::string::npos);
    REQUIRE(s.find("TEST 1581") != std::string::npos);
    REQUIRE(s.find("HELLO81") != std::string::npos);
    REQUIRE(s.find("PRG") != std::string::npos);
    REQUIRE(s.find("BIOS PARAMETER BLOCK") == std::string::npos);
    REQUIRE(s.find("BOOT SECTOR HEX") == std::string::npos);
    REQUIRE(s.find("FAT12") == std::string::npos);
}

TEST_CASE("extract writes D81 PRG payload", "[d81][extract]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_d81_image());
    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "d81-out";
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);

    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 1);
    REQUIRE(err.str().empty());

    std::ifstream prg(dest / "HELLO81.prg", std::ios::binary);
    REQUIRE(prg);
    const std::vector<uint8_t> body((std::istreambuf_iterator<char>(prg)),
                                    std::istreambuf_iterator<char>());
    REQUIRE(body == dumpfloppy_test::sample_d81_prg_bytes());
}

TEST_CASE("update refuses D81/CBMFS", "[d81][update]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(make_d81_image());
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.emplace_back("HELLO81.prg");
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == -1);
    REQUIRE(err.str().find("cannot update D64/CBMFS") != std::string::npos);
}

TEST_CASE("load_image .d81 is container d81_c64", "[d81][image]")
{
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto path = dir / "sample.d81";
    const auto bytes = dumpfloppy_test::make_sample_d81();
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded);
    REQUIRE(loaded->container == dumpfloppy::container_kind::d81_c64);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.cbm.present);
    REQUIRE(a.cbm.media == dumpfloppy::cbm_media::d81);
    REQUIRE(a.cbm.disk_name == "TEST 1581");
    REQUIRE(a.image.size_geometry.media_name.find("1581") != std::string::npos);
}
