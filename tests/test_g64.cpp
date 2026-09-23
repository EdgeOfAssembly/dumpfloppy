/**
 * @file test_g64.cpp
 * @brief GCR-1541 G64 decode, analyse skip-FAT, extract, refuse -u.
 */
#include "g64_builder.hpp"
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/cbm.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/formats/archiveteam/at_g64.h"
#include "dumpfloppy/g64.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/update.hpp"
#include "dumpfloppy/util.hpp"
#include "image_builder.hpp"

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

dumpfloppy::floppy_image make_g64_image()
{
    dumpfloppy::floppy_image img{};
    img.bytes = dumpfloppy_test::make_sample_g64();
    img.path = "sample.g64";
    img.container = dumpfloppy::container_kind::g64_c64;
    return img;
}

std::filesystem::path write_temp_g64()
{
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto path = dir / "sample.g64";
    const auto bytes = dumpfloppy_test::make_sample_g64();
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    REQUIRE(out);
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    REQUIRE(out);
    return path;
}

} /* namespace */

TEST_CASE("is_g64_image requires GCR-1541 version 0", "[g64]")
{
    REQUIRE_FALSE(dumpfloppy::is_g64_image({}));
    const auto g64 = dumpfloppy_test::make_sample_g64();
    REQUIRE(dumpfloppy::is_g64_image(g64));
    auto bad_magic = g64;
    bad_magic[0] = 'X';
    REQUIRE_FALSE(dumpfloppy::is_g64_image(bad_magic));
    auto bad_ver = g64;
    bad_ver[8] = 1;
    REQUIRE_FALSE(dumpfloppy::is_g64_image(bad_ver));
}

TEST_CASE("g64_decode_d64 round-trips sample D64 sectors", "[g64]")
{
    const auto d64 = dumpfloppy_test::make_sample_d64();
    const auto g64 = dumpfloppy_test::d64_to_g64(d64);
    const auto back = dumpfloppy::g64_decode_d64(g64);
    REQUIRE(back.size() == dumpfloppy::k_d64_35_bytes);
    REQUIRE(back == d64);
}

TEST_CASE("parse_g64 fills CBMFS from decoded BAM", "[g64]")
{
    const auto g64 = dumpfloppy_test::make_sample_g64();
    const dumpfloppy::cbm_disk disk = dumpfloppy::parse_g64(g64);
    REQUIRE(disk.present);
    REQUIRE(disk.media == dumpfloppy::cbm_media::g64);
    REQUIRE(disk.media_name == "G64");
    REQUIRE(disk.disk_name == "TEST DISK");
    REQUIRE(disk.disk_id == "DF");
    REQUIRE(disk.decoded.size() == dumpfloppy::k_d64_35_bytes);
    REQUIRE(disk.entries.size() == 2u);
}

TEST_CASE("parse_g64 rejects non-G64", "[g64]")
{
    const auto d64 = dumpfloppy_test::make_sample_d64();
    REQUIRE_FALSE(dumpfloppy::parse_g64(d64).present);
    REQUIRE_FALSE(dumpfloppy::parse_g64({}).present);
}

TEST_CASE("analyse synthetic G64 skips FAT and fills CBM", "[g64][analyse]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(make_g64_image());
    REQUIRE(a.cbm.present);
    REQUIRE(a.cbm.media == dumpfloppy::cbm_media::g64);
    REQUIRE(a.cbm.disk_name == "TEST DISK");
    REQUIRE(a.cbm.entries.size() == 2u);
    REQUIRE_FALSE(a.bpb.looks_valid);
    REQUIRE(a.kind == dumpfloppy::fat_kind::unknown);
    REQUIRE(a.entries.empty());
    REQUIRE_FALSE(a.flux.present);
    REQUIRE(a.image.size_geometry.media_name.find("G64") != std::string::npos);
}

TEST_CASE("FAT12 sample is not G64", "[g64][fat]")
{
    dumpfloppy::floppy_image img{};
    img.bytes = dumpfloppy_test::make_fat12_sample();
    img.path = "sample.ima";
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(img));
    REQUIRE_FALSE(a.cbm.present);
    REQUIRE(a.kind == dumpfloppy::fat_kind::fat12);
}

TEST_CASE("G64 header without CBMFS still skips FAT", "[g64][analyse]")
{
    auto g64 = dumpfloppy_test::make_sample_g64();
    /* Wipe track 18 (BAM) GCR so CBMFS is empty; header stays valid. */
    const uint32_t off = dumpfloppy::read_le32(
        g64, dumpfloppy::k_g64_prefix_bytes + static_cast<std::size_t>(17u * 2u) * 4u);
    REQUIRE(off != 0u);
    const uint16_t actual = dumpfloppy::read_le16(g64, static_cast<std::size_t>(off));
    std::fill(g64.begin() + static_cast<std::ptrdiff_t>(off) + 2,
              g64.begin() + static_cast<std::ptrdiff_t>(off) + 2 + actual, 0);
    dumpfloppy::floppy_image img{};
    img.bytes = std::move(g64);
    img.path = "empty.g64";
    img.container = dumpfloppy::container_kind::g64_c64;
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(img));
    REQUIRE(a.cbm.present);
    REQUIRE(a.cbm.media == dumpfloppy::cbm_media::g64);
    REQUIRE(a.cbm.entries.empty());
    REQUIRE_FALSE(a.bpb.looks_valid);
    REQUIRE(a.kind == dumpfloppy::fat_kind::unknown);
}

TEST_CASE("write_report lists G64 CBM disk name and PRG", "[g64][report]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_g64_image());
    dumpfloppy::report_options opt{};
    opt.color = false;
    opt.hex_boot = true;
    std::ostringstream plain;
    dumpfloppy::write_report(a, plain, opt);
    const std::string s = plain.str();
    REQUIRE(s.find("C64 G64 / CBMFS") != std::string::npos);
    REQUIRE(s.find("1541 G64") != std::string::npos);
    REQUIRE(s.find("TEST DISK") != std::string::npos);
    REQUIRE(s.find("HELLO") != std::string::npos);
    REQUIRE(s.find("PRG") != std::string::npos);
    REQUIRE(s.find("BIOS PARAMETER BLOCK") == std::string::npos);
    REQUIRE(s.find("BOOT SECTOR HEX") == std::string::npos);
}

TEST_CASE("extract writes G64 PRG payload from decoded sectors", "[g64][extract]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_g64_image());
    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "g64-out";
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);

    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 2);
    REQUIRE(err.str().empty());

    std::ifstream prg(dest / "HELLO.prg", std::ios::binary);
    REQUIRE(prg);
    const std::vector<uint8_t> body((std::istreambuf_iterator<char>(prg)),
                                    std::istreambuf_iterator<char>());
    REQUIRE(body == dumpfloppy_test::sample_prg_bytes());
}

TEST_CASE("update refuses G64/CBMFS", "[g64][update]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(make_g64_image());
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.emplace_back("HELLO.prg");
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == -1);
    REQUIRE(err.str().find("cannot update CBMFS") != std::string::npos);
}

TEST_CASE("load_image .g64 is container g64_c64", "[g64][image]")
{
    const auto path = write_temp_g64();
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded);
    REQUIRE(loaded->container == dumpfloppy::container_kind::g64_c64);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.cbm.present);
    REQUIRE(a.cbm.disk_name == "TEST DISK");
}

TEST_CASE("at_g64 detect matches GCR-1541 magic", "[g64][format]")
{
    dumpfloppy::formats::at_g64 fmt{};
    const auto g64 = dumpfloppy_test::make_sample_g64();
    REQUIRE(fmt.detect(g64));
    REQUIRE(fmt.type() == "C64 G64");
    REQUIRE_FALSE(fmt.detect(dumpfloppy_test::make_sample_d64()));
}
