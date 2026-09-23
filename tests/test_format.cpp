/**
 * @file test_format.cpp
 * @brief Abstract format default type and FAT12 disk detect.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/catalog.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/format.h"
#include "dumpfloppy/format_registry.hpp"
#include "dumpfloppy/formats/arc.h"
#include "dumpfloppy/formats/archiveteam/at_trd.h"
#include "dumpfloppy/formats/fat12.h"
#include "dumpfloppy/formats/pkd.h"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/util.hpp"
#include "image_builder.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

TEST_CASE("file_format::type defaults to DATA", "[format]")
{
    dumpfloppy::file_format base{};
    REQUIRE(base.type() == "DATA");
    REQUIRE_FALSE(base.detect({}));
}

TEST_CASE("clip_type_label respects 24-char column", "[format]")
{
    REQUIRE(dumpfloppy::clip_type_label("DATA") == "DATA");
    REQUIRE(dumpfloppy::clip_type_label("DUNE 2000 FONT").size() == 14);
    REQUIRE(dumpfloppy::clip_type_label("HS PACK ARC").size() == 11);
    const std::string long_label(30, 'A');
    REQUIRE(dumpfloppy::clip_type_label(long_label).size() == 24);
}

TEST_CASE("FAT12 detector matches a synthetic 32K volume", "[format]")
{
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    dumpfloppy::formats::fat12 fmt{};
    REQUIRE(fmt.type() == "FAT12");
    REQUIRE(fmt.kind() == dumpfloppy::format_kind::disk_image);
    REQUIRE(fmt.registry() == dumpfloppy::format_registry_id::filesystem);
    REQUIRE(fmt.detect(bytes));
    REQUIRE(dumpfloppy::identify_type(bytes, dumpfloppy::format_kind::disk_image) ==
            "FAT12");
}

TEST_CASE("format catalogs split payload container filesystem", "[format][registry]")
{
    const auto& pay = dumpfloppy::payload_formats();
    const auto& cont = dumpfloppy::container_formats();
    const auto& fs = dumpfloppy::filesystem_formats();
    const auto all = dumpfloppy::all_formats();
    REQUIRE(pay.size() >= 4u);
    REQUIRE(cont.size() >= 2u);
    REQUIRE(fs.size() >= 1u);
    REQUIRE(all.size() == pay.size() + cont.size() + fs.size());
    REQUIRE(fs.front()->type() == "FAT12");
    REQUIRE(fs.front()->registry() == dumpfloppy::format_registry_id::filesystem);

    bool saw_g64 = false;
    bool saw_cbmfs = false;
    bool saw_pkd = false;
    for (const dumpfloppy::file_format* f : pay)
    {
        REQUIRE(f != nullptr);
        REQUIRE(f->kind() == dumpfloppy::format_kind::file);
        REQUIRE(f->registry() == dumpfloppy::format_registry_id::payload);
        if (f->type() == "HS PACK ARC")
        {
            saw_pkd = true;
        }
    }
    for (const dumpfloppy::file_format* f : cont)
    {
        REQUIRE(f != nullptr);
        REQUIRE(f->kind() == dumpfloppy::format_kind::disk_image);
        REQUIRE(f->registry() == dumpfloppy::format_registry_id::container);
        if (f->type() == "C64 G64")
        {
            saw_g64 = true;
        }
    }
    for (const dumpfloppy::file_format* f : fs)
    {
        REQUIRE(f != nullptr);
        REQUIRE(f->kind() == dumpfloppy::format_kind::disk_image);
        REQUIRE(f->registry() == dumpfloppy::format_registry_id::filesystem);
        if (f->type() == "CBMFS")
        {
            saw_cbmfs = true;
        }
    }
    REQUIRE(saw_g64);
    REQUIRE(saw_cbmfs);
    REQUIRE(saw_pkd);
}

TEST_CASE("xxh64_hex is 16 lowercase hex chars", "[format]")
{
    const std::vector<uint8_t> hello{'H', 'e', 'l', 'l', 'o', ',', ' ', 'f',
                                     'l', 'o', 'p', 'p', 'y', '\n'};
    REQUIRE(dumpfloppy::xxh64_hex(hello) == "a41fb567443800ac");
}

TEST_CASE("catalog lookup by whole-image XXH64", "[format][catalog]")
{
    const auto hit = dumpfloppy::catalog_lookup("93a5a1a9002057dd");
    REQUIRE(hit.found);
    REQUIRE(hit.title.find("Commando") != std::string::npos);
    REQUIRE(hit.protection.find("AH=10h") != std::string::npos);
    REQUIRE_FALSE(dumpfloppy::catalog_lookup("0000000000000000").found);
}

TEST_CASE("catalog names C64 Paranoid EA Ocean and Origin protection", "[format][catalog]")
{
    const auto ninja_a = dumpfloppy::catalog_lookup("be2324a14653936b");
    REQUIRE(ninja_a.found);
    REQUIRE(ninja_a.title.find("Last Ninja") != std::string::npos);
    REQUIRE(ninja_a.protection.find("Paranoid") != std::string::npos);

    const auto ninja_b = dumpfloppy::catalog_lookup("00b6c60995d66466");
    REQUIRE(ninja_b.found);
    REQUIRE(ninja_b.protection.find("Paranoid") != std::string::npos);

    const auto archon = dumpfloppy::catalog_lookup("e3ce744e13387bd8");
    REQUIRE(archon.found);
    REQUIRE(archon.title.find("Archon") != std::string::npos);
    REQUIRE(archon.protection.find("half-track 34.5") != std::string::npos);

    const auto batman_c64 = dumpfloppy::catalog_lookup("752b75a5b8a31147");
    REQUIRE(batman_c64.found);
    REQUIRE(batman_c64.title.find("Ocean") != std::string::npos);
    REQUIRE(batman_c64.protection.find("track 36") != std::string::npos);

    const auto origin_mfm = dumpfloppy::catalog_lookup("bd833a724a08fbc5");
    REQUIRE(origin_mfm.found);
    REQUIRE(origin_mfm.title.find("2400") != std::string::npos);
    REQUIRE(origin_mfm.protection.find("C6:H0:S170") != std::string::npos);

    const auto origin_86f = dumpfloppy::catalog_lookup("6f25dadaa3091031");
    REQUIRE(origin_86f.found);
    REQUIRE(origin_86f.protection.find("Origin") != std::string::npos);

    const auto im_crack = dumpfloppy::catalog_lookup("8955f9748027eef3");
    REQUIRE(im_crack.found);
    REQUIRE(im_crack.protection.find("Rapidlok removed") != std::string::npos);
}

TEST_CASE("Last Ninja Side A D64 report catalogues Paranoid", "[format][catalog][optional]")
{
    const std::filesystem::path img{
        "/mnt/dumpfloppy-fixtures/c64/Last_Ninja_The_1987_System_3_Side_A.d64"};
    if (!std::filesystem::exists(img))
    {
        SKIP("Last Ninja Side A .d64 is not present");
    }
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    REQUIRE(loaded->xxh64 == "be2324a14653936b");
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.catalog.found);
    REQUIRE(a.catalog.protection.find("Paranoid") != std::string::npos);
    REQUIRE(a.cbm.disk_name.find("PARANO") != std::string::npos);
    dumpfloppy::report_options opt{};
    opt.color = false;
    opt.hex_boot = false;
    std::ostringstream plain;
    dumpfloppy::write_report(a, plain, opt);
    const std::string text = plain.str();
    REQUIRE(text.find("CATALOG") != std::string::npos);
    REQUIRE(text.find("Paranoid") != std::string::npos);
    REQUIRE(text.find("Protection") != std::string::npos);
}

TEST_CASE("Archon G64 report catalogues EA half-track protection", "[format][catalog][optional]")
{
    const std::filesystem::path img{
        "/mnt/dumpfloppy-fixtures/c64/"
        "Archon (102402)(Electronic Arts, Inc.)(1983) [E1DAD185].g64"};
    if (!std::filesystem::exists(img))
    {
        SKIP("Archon .g64 is not present");
    }
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    REQUIRE(loaded->xxh64 == "e3ce744e13387bd8");
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.catalog.found);
    REQUIRE(a.catalog.protection.find("half-track 34.5") != std::string::npos);
    REQUIRE(a.cbm.present);
}

TEST_CASE("2400 A.D. MFM report catalogues Origin HLS", "[format][catalog][optional]")
{
    const std::filesystem::path img{
        "/mnt/dumpfloppy-fixtures/"
        "2400 A.D. (1988) (ORIGIN Systems, Inc.) (360K) [cp cr] [!]/"
        "2400 A.D. (1988) (ORIGIN Systems, Inc.) (360K) [cp] [!].mfm"};
    if (!std::filesystem::exists(img))
    {
        SKIP("2400 A.D. .mfm is not present");
    }
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    REQUIRE(loaded->xxh64 == "bd833a724a08fbc5");
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.catalog.found);
    REQUIRE(a.catalog.protection.find("C6:H0:S170") != std::string::npos);
}

TEST_CASE("Commando HxC dump is catalogued with CRC protection", "[format][commando]")
{
    const std::filesystem::path img{
        "/tmp/Commando (Booter) (1986) (Data East USA, Inc.) (180K) [cp] [!]/"
        "Commando (Booter) (1986) (Data East USA, Inc.) (180K) [cp] [!].mfm"};
    if (!std::filesystem::exists(img))
    {
        SKIP("Commando .mfm is not present");
    }
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    REQUIRE(loaded->xxh64 == "93a5a1a9002057dd");
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.catalog.found);
    REQUIRE(a.catalog.title.find("Commando") != std::string::npos);
    REQUIRE(a.flux.present);
    REQUIRE(a.flux.tracks == 40);
    REQUIRE(a.flux.sides == 1);
    bool long_sec = false;
    bool bad_crc = false;
    for (const dumpfloppy::ibm_sector& s : a.flux.sectors)
    {
        if (s.cyl == 39 && s.sector == 7 && s.bytes == 1024)
        {
            long_sec = true;
            bad_crc = !s.dam_crc_ok;
        }
    }
    REQUIRE(long_sec);
    REQUIRE(bad_crc);
}

TEST_CASE("Batman Disk 1 MFM is HLS + FAT12 PENGUIN.EXE", "[format][batman]")
{
    const std::filesystem::path img{
        "/tmp/Batman - The Caped Crusader (1989) (Data East USA, Inc.) (360K) [cp] [!]/"
        "Batman - The Caped Crusader (1989) (Data East USA, Inc.) (360K) (Disk 1) [cp] [!].mfm"};
    if (!std::filesystem::exists(img))
    {
        SKIP("Batman Disk 1 .mfm is not present");
    }
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    REQUIRE(loaded->xxh64 == "2091da8694d943f3");
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.catalog.found);
    REQUIRE(a.catalog.title.find("Batman") != std::string::npos);
    REQUIRE(a.catalog.protection.find("HLS") != std::string::npos);
    REQUIRE(a.bpb.looks_valid);
    bool penguin = false;
    for (const dumpfloppy::dir_entry& e : a.entries)
    {
        if (e.name_83 == "PENGUIN.EXE")
        {
            penguin = true;
            REQUIRE(e.type == "EXE");
        }
    }
    REQUIRE(penguin);
    bool s241 = false;
    bool s45 = false;
    for (const dumpfloppy::ibm_sector& s : a.flux.sectors)
    {
        if (s.cyl == 39 && s.head == 0 && s.sector == 241)
        {
            s241 = true;
        }
        if (s.cyl == 39 && s.head == 1 && s.sector == 45 && s.bytes == 256)
        {
            s45 = true;
        }
    }
    REQUIRE(s241);
    REQUIRE(s45);
}

TEST_CASE("SEA ARC magic 0x1A is SEA ARC; Populous ARC is POP ARC", "[format][arc]")
{
    dumpfloppy::formats::sea_arc_header h{};
    h.magic = 0x1A;
    h.method = 8;
    std::memcpy(h.name, "HELLO.TXT", 10);
    h.packed_size = 4;
    h.unpacked_size = 4;
    std::vector<uint8_t> sea(sizeof(h) + 4u);
    std::memcpy(sea.data(), &h, sizeof(h));
    sea[sizeof(h)] = 'A';
    REQUIRE(dumpfloppy::identify_type(sea, dumpfloppy::format_kind::file, "X.ARC") ==
            "SEA ARC");

    std::vector<uint8_t> pop{0x01, 0x00, 0x10, 0xC0, 0x00, 0x00};
    const char* n = "POPULOUS.EXE";
    pop.insert(pop.end(), n, n + 13);
    pop.insert(pop.end(), {0x01, 0x00, 0x5E, 0x28, 0x00, 0x00});
    REQUIRE(dumpfloppy::identify_type(pop, dumpfloppy::format_kind::file, "POPULOUS.ARC") ==
            "POP ARC");
}

TEST_CASE("Populous IMA is catalogued unprotected FAT12", "[format][populous]")
{
    const std::filesystem::path img{
        "/tmp/Populous (1989) (Electronic Arts, Inc.) (360K) [!]/"
        "Populous (1989) (Electronic Arts, Inc.) (360K) [!].ima"};
    if (!std::filesystem::exists(img))
    {
        SKIP("Populous .ima is not present");
    }
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    REQUIRE(loaded->xxh64 == "a4cb00f350c51e63");
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.catalog.found);
    REQUIRE(a.catalog.protection.find("none") != std::string::npos);
    bool saw_arc = false;
    for (const dumpfloppy::dir_entry& e : a.entries)
    {
        if (e.name_83 == "POPULOUS.ARC")
        {
            saw_arc = true;
            REQUIRE(e.type == "POP ARC");
        }
    }
    REQUIRE(saw_arc);
}

TEST_CASE("HxC MFM and 86F magics are disk images", "[format]")
{
    const uint8_t mfm[8] = {'H', 'X', 'C', 'M', 'F', 'M', 0, 40};
    REQUIRE(dumpfloppy::identify_type(mfm, dumpfloppy::format_kind::disk_image) ==
            "HXC MFM");
    const uint8_t f86[8] = {'8', '6', 'B', 'F', 12, 2, 0, 0};
    REQUIRE(dumpfloppy::identify_type(f86, dumpfloppy::format_kind::disk_image) ==
            "86BOX 86F");
}

TEST_CASE("TRD detect requires disk-info signature, not IBM 160K/320K size",
          "[format][trd]")
{
    dumpfloppy::formats::at_trd trd{};
    REQUIRE(trd.type() == "TR-DOS TRD");

    std::vector<uint8_t> ibm160(163840u, 0);
    ibm160[0] = 0xEB;
    ibm160[1] = 0x10;
    ibm160[2] = 0x90;
    ibm160[510] = 0x55;
    ibm160[511] = 0xAA;
    REQUIRE_FALSE(trd.detect(ibm160));
    REQUIRE(dumpfloppy::identify_type(ibm160, dumpfloppy::format_kind::disk_image) !=
            "TR-DOS TRD");

    auto fat160 = dumpfloppy_test::make_fat12_sample();
    fat160.resize(163840u, 0);
    REQUIRE(dumpfloppy::formats::fat12{}.detect(fat160));
    REQUIRE_FALSE(trd.detect(fat160));
    REQUIRE(dumpfloppy::identify_type(fat160, dumpfloppy::format_kind::disk_image) ==
            "FAT12");

    std::vector<uint8_t> ibm320(327680u, 0);
    REQUIRE_FALSE(trd.detect(ibm320));
    REQUIRE(dumpfloppy::identify_type(ibm320, dumpfloppy::format_kind::disk_image) !=
            "TR-DOS TRD");

    std::vector<uint8_t> empty640(655360u, 0);
    REQUIRE_FALSE(trd.detect(empty640));

    auto stamp_trd_info = [](std::vector<uint8_t>& img, uint8_t disk_type, bool with_id)
    {
        constexpr std::size_t k_info = 8u * 256u;
        img[k_info + 0xE1u] = 0; /* first free sector */
        img[k_info + 0xE2u] = 1; /* first free track */
        img[k_info + 0xE3u] = disk_type;
        img[k_info + 0xE4u] = 0; /* file count */
        if (with_id)
        {
            img[k_info + 0xE7u] = 0x10;
        }
    };

    std::vector<uint8_t> trd160(163840u, 0);
    stamp_trd_info(trd160, 0x19u, true);
    REQUIRE(trd.detect(trd160));
    REQUIRE(dumpfloppy::identify_type(trd160, dumpfloppy::format_kind::disk_image) ==
            "TR-DOS TRD");

    std::vector<uint8_t> trd160_geom(163840u, 0);
    stamp_trd_info(trd160_geom, 0x19u, false);
    REQUIRE(trd.detect(trd160_geom));

    std::vector<uint8_t> trd640(655360u, 0);
    stamp_trd_info(trd640, 0x16u, true);
    REQUIRE(trd.detect(trd640));
    REQUIRE(dumpfloppy::identify_type(trd640, dumpfloppy::format_kind::disk_image) ==
            "TR-DOS TRD");
}

TEST_CASE("unknown blob stays DATA", "[format]")
{
    const std::vector<uint8_t> junk{0x00, 0x01, 0x02, 0x03};
    REQUIRE(dumpfloppy::identify_type(junk, dumpfloppy::format_kind::file) == "DATA");
}

TEST_CASE("type stages: DATA then extension then magic, last wins", "[format]")
{
    const std::vector<uint8_t> none{};
    REQUIRE(dumpfloppy::identify_type(none, dumpfloppy::format_kind::file) == "DATA");
    REQUIRE(dumpfloppy::identify_type(none, dumpfloppy::format_kind::file, "GAME.COM") ==
            "COM");
    REQUIRE(dumpfloppy::identify_type(none, dumpfloppy::format_kind::file, "?91.PKD") ==
            "HS PACK ARC");

    std::vector<uint8_t> mz(2, 0);
    mz[0] = 'M';
    mz[1] = 'Z';
    REQUIRE(dumpfloppy::identify_type(mz, dumpfloppy::format_kind::file, "GAME.COM") ==
            "EXE");

    std::vector<uint8_t> aiff(12, 0);
    aiff[0] = 'F';
    aiff[1] = 'O';
    aiff[2] = 'R';
    aiff[3] = 'M';
    aiff[8] = 'A';
    aiff[9] = 'I';
    aiff[10] = 'F';
    aiff[11] = 'F';
    REQUIRE(dumpfloppy::identify_type(aiff, dumpfloppy::format_kind::file, "X.PKD") ==
            "AIFF");
}

TEST_CASE("plain text is not a Horrorsoft Packed Archive", "[format][pkd]")
{
    const std::vector<uint8_t> hello{'H', 'e', 'l', 'l', 'o', ',', ' ', 'f',
                                     'l', 'o', 'p', 'p', 'y', '\n'};
    dumpfloppy::formats::pkd fmt{};
    REQUIRE(fmt.type() == "HS PACK ARC");
    REQUIRE_FALSE(fmt.detect(hello));
}

TEST_CASE("Elvira 011.PKD decrunches via ScummVM AGOS algorithm", "[format][pkd]")
{
    const std::filesystem::path img{
        "/tmp/Elvira (1990) (Accolade, Inc.) (720K) [!]/"
        "Elvira (1990) (Accolade, Inc.) (720K) (Disk 1) [!].ima"};
    if (!std::filesystem::exists(img))
    {
        SKIP("Elvira Disk 1 image is not present");
    }
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    bool saw = false;
    for (const dumpfloppy::dir_entry& e : a.entries)
    {
        if (e.name_83 == "011.PKD" && !e.deleted)
        {
            saw = true;
            REQUIRE(e.type == "HS PACK ARC");
            const auto bytes =
                dumpfloppy::read_file_contents(a.image.bytes, a.bpb, e);
            REQUIRE(dumpfloppy::formats::pkd_unpacked_size(bytes) == 1410u);
            std::vector<uint8_t> out;
            REQUIRE(dumpfloppy::formats::pkd_decrunch(bytes, out));
            REQUIRE(out.size() == 1410u);
        }
    }
    REQUIRE(saw);
}

TEST_CASE("deleted Elvira PKD keeps HS PACK ARC type", "[format][pkd]")
{
    const std::filesystem::path img{
        "/tmp/Elvira (1990) (Accolade, Inc.) (720K) [!]/"
        "Elvira (1990) (Accolade, Inc.) (720K) (Disk 2) [!].ima"};
    if (!std::filesystem::exists(img))
    {
        SKIP("Elvira Disk 2 image is not present");
    }
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    bool saw = false;
    for (const dumpfloppy::dir_entry& e : a.entries)
    {
        if (e.deleted && e.name_83.size() >= 4 &&
            e.name_83.substr(e.name_83.size() - 4) == ".PKD")
        {
            saw = true;
            REQUIRE(e.type == "HS PACK ARC");
        }
    }
    REQUIRE(saw);
}

TEST_CASE("AIFF magic from the Shikadi catalog", "[format]")
{
    std::vector<uint8_t> aiff(12, 0);
    aiff[0] = 'F';
    aiff[1] = 'O';
    aiff[2] = 'R';
    aiff[3] = 'M';
    aiff[8] = 'A';
    aiff[9] = 'I';
    aiff[10] = 'F';
    aiff[11] = 'F';
    REQUIRE(dumpfloppy::identify_type(aiff, dumpfloppy::format_kind::file) == "AIFF");
}
