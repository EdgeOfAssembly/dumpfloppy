/**
 * @file test_g71.cpp
 * @brief GCR-1571 G71 decode, analyse skip-FAT, extract, refuse -u.
 */
#include "g71_builder.hpp"
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/cbm.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/formats/archiveteam/at_g71.h"
#include "dumpfloppy/g64.hpp"
#include "dumpfloppy/g71.hpp"
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

dumpfloppy::floppy_image make_g71_image()
{
    dumpfloppy::floppy_image img{};
    img.bytes = dumpfloppy_test::make_sample_g71();
    img.path = "sample.g71";
    img.container = dumpfloppy::container_kind::g71_c64;
    return img;
}

std::filesystem::path write_temp_g71()
{
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto path = dir / "sample.g71";
    const auto bytes = dumpfloppy_test::make_sample_g71();
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    REQUIRE(out);
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    REQUIRE(out);
    return path;
}

} /* namespace */

TEST_CASE("is_g71_image requires GCR-1571 version 0", "[g71]")
{
    REQUIRE_FALSE(dumpfloppy::is_g71_image({}));
    const auto g71 = dumpfloppy_test::make_sample_g71();
    REQUIRE(dumpfloppy::is_g71_image(g71));
    REQUIRE_FALSE(dumpfloppy::is_g64_image(g71));
    auto bad_magic = g71;
    bad_magic[0] = 'X';
    REQUIRE_FALSE(dumpfloppy::is_g71_image(bad_magic));
    auto bad_ver = g71;
    bad_ver[8] = 1;
    REQUIRE_FALSE(dumpfloppy::is_g71_image(bad_ver));
    REQUIRE_FALSE(dumpfloppy::is_g71_image(dumpfloppy_test::make_sample_g64()));
}

TEST_CASE("g71_decode_d71 round-trips sample D71 sectors", "[g71]")
{
    const auto d71 = dumpfloppy_test::make_sample_d71();
    const auto g71 = dumpfloppy_test::d71_to_g71(d71);
    const auto back = dumpfloppy::g71_decode_d71(g71);
    REQUIRE(back.size() == dumpfloppy::k_d71_bytes);
    REQUIRE(back == d71);
}

TEST_CASE("g71_decode_d71 round-trips 84-slot GCR-1571", "[g71]")
{
    const auto d71 = dumpfloppy_test::make_sample_d71();
    const auto g71 = dumpfloppy_test::d71_to_g71(d71, 84u);
    REQUIRE(g71[9] == 84u);
    const auto back = dumpfloppy::g71_decode_d71(g71);
    REQUIRE(back.size() == dumpfloppy::k_d71_bytes);
    REQUIRE(back == d71);
}

TEST_CASE("parse_g71 fills CBMFS including side-1 PRG", "[g71]")
{
    const auto g71 = dumpfloppy_test::make_sample_g71();
    const dumpfloppy::cbm_disk disk = dumpfloppy::parse_g71(g71);
    REQUIRE(disk.present);
    REQUIRE(disk.media == dumpfloppy::cbm_media::g71);
    REQUIRE(disk.media_name == "G71");
    REQUIRE(disk.disk_name == "TEST 1571");
    REQUIRE(disk.disk_id == "71");
    REQUIRE(disk.decoded.size() == dumpfloppy::k_d71_bytes);
    REQUIRE(disk.entries.size() == 1u);
    REQUIRE(disk.entries[0].name == "SIDE1");
    REQUIRE(disk.entries[0].first_track == 36u);
    REQUIRE(disk.entries[0].first_sector == 0u);
}

TEST_CASE("parse_g71 rejects non-G71", "[g71]")
{
    REQUIRE_FALSE(dumpfloppy::parse_g71(dumpfloppy_test::make_sample_d71()).present);
    REQUIRE_FALSE(dumpfloppy::parse_g71(dumpfloppy_test::make_sample_g64()).present);
    REQUIRE_FALSE(dumpfloppy::parse_g71({}).present);
}

TEST_CASE("analyse synthetic G71 skips FAT and fills CBM", "[g71][analyse]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(make_g71_image());
    REQUIRE(a.cbm.present);
    REQUIRE(a.cbm.media == dumpfloppy::cbm_media::g71);
    REQUIRE(a.cbm.disk_name == "TEST 1571");
    REQUIRE(a.cbm.entries.size() == 1u);
    REQUIRE_FALSE(a.bpb.looks_valid);
    REQUIRE(a.kind == dumpfloppy::fat_kind::unknown);
    REQUIRE(a.entries.empty());
    REQUIRE_FALSE(a.flux.present);
    REQUIRE(a.image.size_geometry.media_name.find("G71") != std::string::npos);
}

TEST_CASE("G71 header without CBMFS still skips FAT", "[g71][analyse]")
{
    auto g71 = dumpfloppy_test::make_sample_g71();
    /* Wipe track 18 (BAM) GCR so CBMFS is empty; header stays valid. */
    const uint32_t off = dumpfloppy::read_le32(
        g71, dumpfloppy::k_g71_prefix_bytes + static_cast<std::size_t>(17u * 2u) * 4u);
    REQUIRE(off != 0u);
    const uint16_t actual = dumpfloppy::read_le16(g71, static_cast<std::size_t>(off));
    std::fill(g71.begin() + static_cast<std::ptrdiff_t>(off) + 2,
              g71.begin() + static_cast<std::ptrdiff_t>(off) + 2 + actual, 0);
    dumpfloppy::floppy_image img{};
    img.bytes = std::move(g71);
    img.path = "empty.g71";
    img.container = dumpfloppy::container_kind::g71_c64;
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(img));
    REQUIRE(a.cbm.present);
    REQUIRE(a.cbm.media == dumpfloppy::cbm_media::g71);
    REQUIRE(a.cbm.entries.empty());
    REQUIRE_FALSE(a.bpb.looks_valid);
    REQUIRE(a.kind == dumpfloppy::fat_kind::unknown);
}

TEST_CASE("write_report lists G71 CBM disk name and side-1 PRG", "[g71][report]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_g71_image());
    dumpfloppy::report_options opt{};
    opt.color = false;
    opt.hex_boot = true;
    std::ostringstream plain;
    dumpfloppy::write_report(a, plain, opt);
    const std::string s = plain.str();
    REQUIRE(s.find("C64 G71 / CBMFS") != std::string::npos);
    REQUIRE(s.find("CBMFS (Commodore 1571 G71)") != std::string::npos);
    REQUIRE(s.find("; not FAT") == std::string::npos);
    REQUIRE(s.find("1571 G71") != std::string::npos);
    REQUIRE(s.find("TEST 1571") != std::string::npos);
    REQUIRE(s.find("SIDE1") != std::string::npos);
    REQUIRE(s.find("PRG") != std::string::npos);
    REQUIRE(s.find("BIOS PARAMETER BLOCK") == std::string::npos);
    REQUIRE(s.find("BOOT SECTOR HEX") == std::string::npos);
}

TEST_CASE("extract writes G71 PRG payload from decoded side-1 sectors", "[g71][extract]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_g71_image());
    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "g71-out";
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

TEST_CASE("update refuses G71/CBMFS", "[g71][update]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(make_g71_image());
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.emplace_back("SIDE1.prg");
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == -1);
    REQUIRE(err.str().find("cannot update G71") != std::string::npos);
}

TEST_CASE("load_image .g71 is container g71_c64", "[g71][image]")
{
    const auto path = write_temp_g71();
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded);
    REQUIRE(loaded->container == dumpfloppy::container_kind::g71_c64);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.cbm.present);
    REQUIRE(a.cbm.disk_name == "TEST 1571");
}

TEST_CASE("at_g71 detect matches GCR-1571 magic", "[g71][format]")
{
    dumpfloppy::formats::at_g71 fmt{};
    const auto g71 = dumpfloppy_test::make_sample_g71();
    REQUIRE(fmt.detect(g71));
    REQUIRE(fmt.type() == "C64 G71");
    REQUIRE_FALSE(fmt.detect(dumpfloppy_test::make_sample_d71()));
    REQUIRE_FALSE(fmt.detect(dumpfloppy_test::make_sample_g64()));
}

TEST_CASE("analyse 84-slot G71 lists SIDE1", "[g71][analyse]")
{
    dumpfloppy::floppy_image img{};
    img.bytes = dumpfloppy_test::make_sample_g71_84();
    img.path = "sample84.g71";
    img.container = dumpfloppy::container_kind::g71_c64;
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(img));
    REQUIRE(a.cbm.present);
    REQUIRE(a.cbm.media == dumpfloppy::cbm_media::g71);
    REQUIRE(a.cbm.entries.size() == 1u);
    REQUIRE(a.cbm.entries[0].name == "SIDE1");
}
