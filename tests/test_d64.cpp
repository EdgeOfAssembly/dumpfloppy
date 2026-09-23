/**
 * @file test_d64.cpp
 * @brief Product wiring: analyse / listing / extract / refuse -u on D64 CBMFS.
 */
#include "d64_builder.hpp"
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/cbm.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/update.hpp"
#include "image_builder.hpp"

#include <tui/ansi.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace
{

dumpfloppy::floppy_image make_d64_image()
{
    dumpfloppy::floppy_image img{};
    img.bytes = dumpfloppy_test::make_sample_d64();
    img.path = "sample.d64";
    img.container = dumpfloppy::container_kind::d64_c64;
    return img;
}

std::filesystem::path write_temp_d64()
{
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto path = dir / "sample.d64";
    const auto bytes = dumpfloppy_test::make_sample_d64();
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    REQUIRE(out);
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    REQUIRE(out);
    return path;
}

} /* namespace */

TEST_CASE("analyse synthetic D64 skips FAT and fills CBM", "[d64][analyse]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(make_d64_image());
    REQUIRE(a.cbm.present);
    REQUIRE(a.cbm.disk_name == "TEST DISK");
    REQUIRE(a.cbm.disk_id == "DF");
    REQUIRE(a.cbm.entries.size() == 2u);
    REQUIRE_FALSE(a.bpb.looks_valid);
    REQUIRE(a.kind == dumpfloppy::fat_kind::unknown);
    REQUIRE(a.entries.empty());
    REQUIRE_FALSE(a.flux.present);
    REQUIRE(a.boot.kind == dumpfloppy::boot_class::not_bootable);
    REQUIRE_FALSE(a.boot.is_booter);
    REQUIRE(a.image.size_geometry.media_name.find("1541") != std::string::npos);

    bool saw_prg = false;
    bool saw_seq = false;
    for (const dumpfloppy::cbm_file& e : a.cbm.entries)
    {
        if (e.name == "HELLO")
        {
            saw_prg = true;
            REQUIRE(e.kind == dumpfloppy::cbm_file_kind::prg);
            REQUIRE_FALSE(e.deleted);
        }
        if (e.name == "OLDSEQ")
        {
            saw_seq = true;
            REQUIRE(e.kind == dumpfloppy::cbm_file_kind::seq);
            REQUIRE(e.deleted);
        }
    }
    REQUIRE(saw_prg);
    REQUIRE(saw_seq);
}

TEST_CASE("FAT12 sample is not CBMFS", "[d64][fat]")
{
    dumpfloppy::floppy_image img{};
    img.bytes = dumpfloppy_test::make_fat12_sample();
    img.path = "sample.ima";
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(img));
    REQUIRE_FALSE(a.cbm.present);
    REQUIRE_FALSE(a.amiga.present);
    REQUIRE(a.bpb.looks_valid);
    REQUIRE(a.kind == dumpfloppy::fat_kind::fat12);
}

TEST_CASE("write_report lists CBM disk name and PRG", "[d64][report]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_d64_image());
    dumpfloppy::report_options opt{};
    opt.color = false;
    opt.hex_boot = true;
    std::ostringstream plain;
    dumpfloppy::write_report(a, plain, opt);
    const std::string s = plain.str();
    REQUIRE(s.find("C64 D64 / CBMFS") != std::string::npos);
    REQUIRE(s.find("CBMFS (Commodore 1541 D64)") != std::string::npos);
    REQUIRE(s.find("; not FAT") == std::string::npos);
    REQUIRE(s.find("CBMFS") != std::string::npos);
    REQUIRE(s.find("TEST DISK") != std::string::npos);
    REQUIRE(s.find("HELLO") != std::string::npos);
    REQUIRE(s.find("PRG") != std::string::npos);
    REQUIRE(s.find("OLDSEQ") != std::string::npos);
    REQUIRE(s.find("SEQ") != std::string::npos);
    REQUIRE(s.find("BIOS PARAMETER BLOCK") == std::string::npos);
    REQUIRE(s.find("BOOT SECTOR HEX") == std::string::npos);
    REQUIRE(s.find("FAT12") == std::string::npos);
    REQUIRE(s.find("Extended BPB") == std::string::npos);

    opt.show_deleted = false;
    std::ostringstream hide;
    dumpfloppy::write_report(a, hide, opt);
    REQUIRE(hide.str().find("OLDSEQ") == std::string::npos);
    REQUIRE(hide.str().find("HELLO") != std::string::npos);
}

TEST_CASE("deleted CBM rows use light-red and bold white", "[d64][ansi]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_d64_image());
    dumpfloppy::report_options opt{};
    opt.color = true;
    opt.hex_boot = false;
    std::ostringstream colored;
    dumpfloppy::write_report(a, colored, opt);
    const std::string s = colored.str();
    REQUIRE(s.find(TUI_BG_BRIGHT_RED) != std::string::npos);
    REQUIRE(s.find(TUI_WHITE) != std::string::npos);
    REQUIRE(s.find(TUI_BOLD) != std::string::npos);
    REQUIRE(s.find(TUI_BLINK) == std::string::npos);
    REQUIRE(s.find("OLDSEQ") != std::string::npos);
}

TEST_CASE("extract writes CBM PRG payload and deleted SEQ", "[d64][extract]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_d64_image());
    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "d64-out";
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

    std::ifstream seq(dest / "OLDSEQ.seq", std::ios::binary);
    REQUIRE(seq);
    const std::vector<uint8_t> seq_body((std::istreambuf_iterator<char>(seq)),
                                        std::istreambuf_iterator<char>());
    REQUIRE(seq_body == dumpfloppy_test::sample_seq_bytes());
}

TEST_CASE("extract glob matches CBM name and type", "[d64][extract]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_d64_image());
    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "d64-glob";
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);

    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    opt.patterns.emplace_back("*.prg");
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 1);
    REQUIRE(std::filesystem::exists(dest / "HELLO.prg"));
    REQUIRE_FALSE(std::filesystem::exists(dest / "OLDSEQ.seq"));

    opt.patterns = {"SEQ"};
    const auto dest2 = dest / "type";
    opt.dest_dir = dest2;
    std::ostringstream err2;
    REQUIRE(dumpfloppy::extract_files(a, opt, err2) == 1);
    REQUIRE(std::filesystem::exists(dest2 / "OLDSEQ.seq"));
}

TEST_CASE("update same-size D64 PRG in place", "[d64][update]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(make_d64_image());
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto host = dir / "HELLO.prg";
    std::vector<uint8_t> neu = dumpfloppy_test::sample_prg_bytes();
    REQUIRE(neu.size() == 300u);
    std::fill(neu.begin(), neu.end(), static_cast<uint8_t>(0x5A));
    const char tag[] = "UPDATED-PRG";
    std::memcpy(neu.data(), tag, sizeof(tag) - 1u);
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
    const std::vector<uint8_t> got =
        dumpfloppy::read_cbm_file(a.image.bytes, a.cbm.media, a.cbm.entries[0]);
    REQUIRE(got == neu);
}

TEST_CASE("update D64 refuses size mismatch and deleted names", "[d64][update]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(make_d64_image());
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto host = dir / "HELLO.prg";
    {
        std::ofstream out(host, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out << "too-short";
    }
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.push_back(host);
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == -1);
    REQUIRE(err.str().find("same-size") != std::string::npos);

    dumpfloppy::analysis b = dumpfloppy::analyse(make_d64_image());
    opt.hosts = {dir / "OLDSEQ.seq"};
    {
        std::ofstream out(opt.hosts[0], std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out << "SEQ-DATA";
    }
    std::ostringstream err2;
    REQUIRE(dumpfloppy::update_files(b, opt, err2) == -1);
    REQUIRE(err2.str().find("no live CBM file") != std::string::npos);
}

TEST_CASE("load_image .d64 is container d64_c64", "[d64][image]")
{
    const auto path = write_temp_d64();
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded);
    REQUIRE(loaded->container == dumpfloppy::container_kind::d64_c64);
    REQUIRE(loaded->size_geometry.media_name.find("1541") != std::string::npos);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.cbm.present);
    REQUIRE(a.cbm.disk_name == "TEST DISK");
}

TEST_CASE("optional real D64 images analyse as CBMFS", "[d64][optional]")
{
    const std::filesystem::path ninja{"/mnt/RetroCodeMess/c64/last_ninja.d64"};
    const std::filesystem::path karate{
        "/mnt/Lataukset/Karateka_Jordan_Mechner_Copy_1985-05-02.d64"};
    std::vector<std::filesystem::path> paths;
    if (std::filesystem::exists(ninja))
    {
        paths.push_back(ninja);
    }
    if (std::filesystem::exists(karate))
    {
        paths.push_back(karate);
    }
    if (paths.empty())
    {
        SKIP("Last Ninja / Karateka .d64 is not present");
    }
    for (const std::filesystem::path& path : paths)
    {
        auto loaded = dumpfloppy::load_image(path);
        REQUIRE(loaded);
        const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
        REQUIRE(a.cbm.present);
        REQUIRE_FALSE(a.bpb.looks_valid);
        REQUIRE(a.entries.empty());
        REQUIRE_FALSE(a.flux.present);
        const bool named =
            a.cbm.disk_name.find("DIGITAL") != std::string::npos ||
            a.cbm.disk_name.find("KARATEKA") != std::string::npos;
        REQUIRE(named);
    }
}
