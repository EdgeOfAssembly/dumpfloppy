/**
 * @file test_foreign.cpp
 * @brief IPF / WOZ / STX / 2IMG skip FAT and report container metadata.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/catalog.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/foreign.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/formats/archiveteam/at_woz.h"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/update.hpp"
#include "image_builder.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace
{

void put_be32(std::vector<uint8_t>& b, std::size_t off, uint32_t v)
{
    b[off] = static_cast<uint8_t>(v >> 24);
    b[off + 1u] = static_cast<uint8_t>(v >> 16);
    b[off + 2u] = static_cast<uint8_t>(v >> 8);
    b[off + 3u] = static_cast<uint8_t>(v);
}

void put_le32(std::vector<uint8_t>& b, std::size_t off, uint32_t v)
{
    b[off] = static_cast<uint8_t>(v);
    b[off + 1u] = static_cast<uint8_t>(v >> 8);
    b[off + 2u] = static_cast<uint8_t>(v >> 16);
    b[off + 3u] = static_cast<uint8_t>(v >> 24);
}

std::vector<uint8_t> make_ipf()
{
    /* CAPS 12 + INFO 96 + one IMGE 80. */
    std::vector<uint8_t> img(12u + 96u + 80u, 0);
    std::memcpy(img.data(), "CAPS", 4);
    put_be32(img, 4, 12u);
    std::memcpy(img.data() + 12, "INFO", 4);
    put_be32(img, 16, 96u);
    const std::size_t p = 24;
    put_be32(img, p + 12u, 1357u); /* file id */
    put_be32(img, p + 24u, 0u);
    put_be32(img, p + 28u, 81u);
    put_be32(img, p + 32u, 0u);
    put_be32(img, p + 36u, 1u);
    put_be32(img, p + 48u, 1u); /* Amiga */
    std::memcpy(img.data() + 108, "IMGE", 4);
    put_be32(img, 112, 80u);
    return img;
}

std::vector<uint8_t> make_woz()
{
    std::vector<uint8_t> img(12u + 8u + 60u, 0);
    std::memcpy(img.data(), "WOZ2", 4);
    img[4] = 0xFFu;
    img[5] = 0x0Au;
    img[6] = 0x0Du;
    img[7] = 0x0Au;
    std::memcpy(img.data() + 12, "INFO", 4);
    put_le32(img, 16, 60u);
    img[20] = 2; /* version */
    img[21] = 1; /* 5.25 */
    img[22] = 1; /* write protected */
    const char* cr = "Applesauce";
    std::memcpy(img.data() + 25, cr, 10);
    return img;
}

std::vector<uint8_t> make_stx()
{
    std::vector<uint8_t> img(16, 0);
    img[0] = 'R';
    img[1] = 'S';
    img[2] = 'Y';
    img[3] = 0;
    img[4] = 0x00;
    img[5] = 0x03;
    img[10] = 80;
    return img;
}

std::vector<uint8_t> make_2img()
{
    std::vector<uint8_t> img(64, 0);
    std::memcpy(img.data(), "2IMG", 4);
    std::memcpy(img.data() + 4, "B2TR", 4);
    img[8] = 64;
    img[12] = 1; /* ProDOS */
    put_le32(img, 22, 64u);
    put_le32(img, 26, 0u);
    return img;
}

std::vector<uint8_t> fat_to_stx(const std::vector<uint8_t>& fat)
{
    constexpr uint8_t spt = 8;
    const uint32_t nsec = static_cast<uint32_t>(fat.size() / 512u);
    const uint8_t tracks = static_cast<uint8_t>(nsec / spt);
    std::vector<uint8_t> stx(16, 0);
    stx[0] = 'R';
    stx[1] = 'S';
    stx[2] = 'Y';
    stx[4] = 0x00;
    stx[5] = 0x03;
    stx[6] = 0x01;
    stx[10] = tracks;
    stx[11] = 0x02;
    for (uint8_t tr = 0; tr < tracks; ++tr)
    {
        const uint32_t rec = 16u + static_cast<uint32_t>(spt) * 16u +
                             static_cast<uint32_t>(spt) * 512u;
        const std::size_t t0 = stx.size();
        stx.resize(t0 + rec, 0);
        put_le32(stx, t0, rec);
        put_le32(stx, t0 + 4u, 0);
        stx[t0 + 8u] = spt;
        stx[t0 + 9u] = 0;
        stx[t0 + 10u] = 0x01; /* sector descriptors */
        stx[t0 + 11u] = 0;
        stx[t0 + 14u] = tr;
        for (uint8_t s = 1; s <= spt; ++s)
        {
            const std::size_t d = t0 + 16u + static_cast<std::size_t>(s - 1u) * 16u;
            put_le32(stx, d, static_cast<uint32_t>(s - 1u) * 512u);
            stx[d + 8u] = tr;
            stx[d + 9u] = 0;
            stx[d + 10u] = s;
            stx[d + 11u] = 2;
        }
        const std::size_t data = t0 + 16u + static_cast<std::size_t>(spt) * 16u;
        const std::size_t src = static_cast<std::size_t>(tr) * spt * 512u;
        std::memcpy(stx.data() + data, fat.data() + src, static_cast<std::size_t>(spt) * 512u);
    }
    return stx;
}

dumpfloppy::floppy_image wrap(std::vector<uint8_t> bytes, const char* path)
{
    dumpfloppy::floppy_image img{};
    img.bytes = std::move(bytes);
    img.path = path;
    return img;
}

} /* namespace */

TEST_CASE("WOZ magic is APPLE WOZ", "[foreign][woz]")
{
    dumpfloppy::formats::at_woz woz{};
    REQUIRE_FALSE(woz.detect({}));
    const auto img = make_woz();
    REQUIRE(woz.detect(img));
    const dumpfloppy::foreign_disk d = dumpfloppy::parse_foreign(img);
    REQUIRE(d.present);
    REQUIRE(d.kind == dumpfloppy::foreign_kind::woz);
    REQUIRE(d.write_protected);
}

TEST_CASE("analyse IPF skips FAT and names SPS", "[foreign][ipf]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(wrap(make_ipf(), "t.ipf"));
    REQUIRE(a.foreign.present);
    REQUIRE(a.foreign.format == "SPS IPF");
    REQUIRE(a.foreign.platform == "Amiga");
    REQUIRE(a.foreign.file_id == 1357u);
    REQUIRE(a.foreign.track_count == 1u);
    REQUIRE_FALSE(a.bpb.looks_valid);
    REQUIRE(a.entries.empty());
    dumpfloppy::report_options opt{};
    opt.color = false;
    opt.hex_boot = true;
    std::ostringstream plain;
    dumpfloppy::write_report(a, plain, opt);
    const std::string s = plain.str();
    REQUIRE(s.find("SPS IPF") != std::string::npos);
    REQUIRE(s.find("Amiga") != std::string::npos);
    REQUIRE(s.find("BIOS PARAMETER BLOCK") == std::string::npos);
    REQUIRE(s.find("BOOT SECTOR HEX") == std::string::npos);
}

TEST_CASE("analyse STX and 2IMG skip FAT", "[foreign]")
{
    const dumpfloppy::analysis stx = dumpfloppy::analyse(wrap(make_stx(), "t.stx"));
    REQUIRE(stx.foreign.present);
    REQUIRE(stx.foreign.kind == dumpfloppy::foreign_kind::stx);
    REQUIRE(stx.foreign.track_count == 80u);
    REQUIRE_FALSE(stx.bpb.looks_valid);

    const dumpfloppy::analysis mg = dumpfloppy::analyse(wrap(make_2img(), "t.2mg"));
    REQUIRE(mg.foreign.present);
    REQUIRE(mg.foreign.kind == dumpfloppy::foreign_kind::img2mg);
    REQUIRE(mg.foreign.creator.find("B2TR") != std::string::npos);
}

TEST_CASE("extract and update refuse IPF", "[foreign][extract]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(wrap(make_ipf(), "t.ipf"));
    dumpfloppy::extract_options x{};
    x.enabled = true;
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, x, err) != 0);
    REQUIRE(err.str().find("SPS IPF") != std::string::npos);
    dumpfloppy::update_options u{};
    u.enabled = true;
    u.hosts.push_back("x");
    std::ostringstream uerr;
    REQUIRE(dumpfloppy::update_files(a, u, uerr) != 0);
}

TEST_CASE("STX standard 512-byte sectors list GEMDOS FAT12", "[foreign][stx]")
{
    const auto fat = dumpfloppy_test::make_fat12_sample();
    const dumpfloppy::analysis a =
        dumpfloppy::analyse(wrap(fat_to_stx(fat), "t.stx"));
    REQUIRE(a.foreign.present);
    REQUIRE(a.foreign.kind == dumpfloppy::foreign_kind::stx);
    REQUIRE_FALSE(a.flux.assembled_chs.empty());
    REQUIRE(a.bpb.looks_valid);
    REQUIRE(a.kind == dumpfloppy::fat_kind::fat12);
    bool hello = false;
    for (const dumpfloppy::dir_entry& e : a.entries)
    {
        if (e.name_83 == "HELLO.TXT")
        {
            hello = true;
        }
    }
    REQUIRE(hello);
    dumpfloppy::report_options opt{};
    opt.color = false;
    std::ostringstream plain;
    dumpfloppy::write_report(a, plain, opt);
    const std::string s = plain.str();
    REQUIRE(s.find("ATARI STX / FAT12") != std::string::npos);
    REQUIRE(s.find("GEMDOS") != std::string::npos);
    REQUIRE(s.find("HELLO.TXT") != std::string::npos);
    REQUIRE(s.find("CONTAINER") != std::string::npos);
}

TEST_CASE("extract HELLO.TXT from assembled STX", "[foreign][stx][extract]")
{
    const auto fat = dumpfloppy_test::make_fat12_sample();
    const dumpfloppy::analysis a =
        dumpfloppy::analyse(wrap(fat_to_stx(fat), "t.stx"));
    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "stx-out";
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);
    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) >= 1);
    REQUIRE(std::filesystem::exists(dest / "HELLO.TXT"));
}

TEST_CASE("catalog names Shadow of the Beast IPF Copylock", "[foreign][catalog]")
{
    const auto d1 = dumpfloppy::catalog_lookup("f7f7c9d577fd9aa2");
    REQUIRE(d1.found);
    REQUIRE(d1.title.find("Beast") != std::string::npos);
    REQUIRE(d1.protection.find("Copylock") != std::string::npos);
    const auto d2 = dumpfloppy::catalog_lookup("fcad06bacfaba8d4");
    REQUIRE(d2.found);
}

TEST_CASE("optional SOTB Disk 1 IPF catalogues Copylock", "[foreign][optional]")
{
    const std::filesystem::path img{
        "/mnt/dumpfloppy-fixtures/amiga/ShadowOfTheBeast_Disk1.ipf"};
    if (!std::filesystem::exists(img))
    {
        SKIP("SOTB Disk 1 .ipf is not present");
    }
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    REQUIRE(loaded->xxh64 == "f7f7c9d577fd9aa2");
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.foreign.present);
    REQUIRE(a.catalog.found);
    REQUIRE(a.catalog.protection.find("Copylock") != std::string::npos);
    REQUIRE(a.foreign.platform == "Amiga");
}
