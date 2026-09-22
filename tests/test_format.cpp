/**
 * @file test_format.cpp
 * @brief Abstract format default type and FAT12 disk detect.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/format.h"
#include "dumpfloppy/format_registry.hpp"
#include "dumpfloppy/formats/fat12.h"
#include "dumpfloppy/formats/pkd.h"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/util.hpp"
#include "image_builder.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
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
    REQUIRE(fmt.detect(bytes));
    REQUIRE(dumpfloppy::identify_type(bytes, dumpfloppy::format_kind::disk_image) ==
            "FAT12");
}

TEST_CASE("xxh64_hex is 16 lowercase hex chars", "[format]")
{
    const std::vector<uint8_t> hello{'H', 'e', 'l', 'l', 'o', ',', ' ', 'f',
                                     'l', 'o', 'p', 'p', 'y', '\n'};
    REQUIRE(dumpfloppy::xxh64_hex(hello) == "a41fb567443800ac");
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
