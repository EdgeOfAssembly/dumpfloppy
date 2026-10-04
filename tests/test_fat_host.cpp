/**
 * @file test_fat_host.cpp
 * @brief Escaped OEM / --offset names, and FAT extract that keeps subdirectories.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/offset.hpp"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/util.hpp"
#include "image_builder.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <vector>

namespace
{

std::filesystem::path write_temp(const std::vector<uint8_t>& bytes, const std::string& name)
{
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto path = dir / name;
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    REQUIRE(out);
    if (!bytes.empty())
    {
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }
    REQUIRE(out);
    return path;
}

dumpfloppy::analysis analyse_bytes(const std::vector<uint8_t>& bytes, const std::string& name)
{
    const auto img = write_temp(bytes, name);
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    return dumpfloppy::analyse(std::move(*loaded));
}

std::filesystem::path scratch(const char* name)
{
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests" / name;
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir;
}

std::string slurp_file(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    REQUIRE(in);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::string line_containing(std::string_view report, std::string_view needle)
{
    std::string cur;
    for (size_t i = 0; i <= report.size(); ++i)
    {
        if (i == report.size() || report[i] == '\n')
        {
            if (cur.find(needle) != std::string::npos)
            {
                return cur;
            }
            cur.clear();
        }
        else
        {
            cur.push_back(report[i]);
        }
    }
    return {};
}

int count_regular_files(const std::filesystem::path& root)
{
    int n = 0;
    if (!std::filesystem::exists(root))
    {
        return 0;
    }
    for (const auto& ent : std::filesystem::recursive_directory_iterator(root))
    {
        if (ent.is_regular_file())
        {
            ++n;
        }
    }
    return n;
}

std::string slurp_popen(const std::string& cmd, int& rc)
{
    rc = -1;
    FILE* fp = popen(cmd.c_str(), "r");
    if (fp == nullptr)
    {
        return {};
    }
    std::string out;
    char buf[4096];
    while (fgets(buf, sizeof(buf), fp) != nullptr)
    {
        out += buf;
    }
    const int closed = pclose(fp);
    if (closed == -1)
    {
        rc = -1;
        return out;
    }
    if (WIFEXITED(closed))
    {
        rc = WEXITSTATUS(closed);
    }
    else
    {
        rc = closed;
    }
    return out;
}

const char* bin_or_require()
{
    const char* bin = std::getenv("DUMPFLOPPY_BIN");
    REQUIRE(bin != nullptr);
    REQUIRE(bin[0] != '\0');
    return bin;
}

dumpfloppy::dir_entry payload(std::string path, std::string name83)
{
    dumpfloppy::dir_entry e{};
    e.path = std::move(path);
    e.name_83 = std::move(name83);
    e.attributes = dumpfloppy::k_attr_archive;
    return e;
}

int extract_all(const dumpfloppy::analysis& a, const std::filesystem::path& dest,
                std::string& err_text)
{
    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    const int n = dumpfloppy::extract_files(a, opt, err);
    err_text = err.str();
    return n;
}

} /* namespace */

TEST_CASE("escape_name uses \\xNN outside 0x20-0x7E", "[names][escape]")
{
    REQUIRE(dumpfloppy::escape_name("HELLO.TXT") == "HELLO.TXT");
    REQUIRE(dumpfloppy::escape_name(" ~") == " ~");
    REQUIRE(dumpfloppy::escape_name("").empty());
    REQUIRE(dumpfloppy::escape_name("\x1b]0;OEM\x07") == "\\x1b]0;OEM\\x07");
    REQUIRE(dumpfloppy::escape_name("\x1f\x7f") == "\\x1f\\x7f");
    REQUIRE(dumpfloppy::escape_name(std::string("\x1b", 1)).find('\x1b') == std::string::npos);
}

TEST_CASE("report escapes OEM and a directory name; empty OEM stays (none)",
          "[names][report][oem]")
{
    const dumpfloppy::analysis esc =
        analyse_bytes(dumpfloppy_test::make_fat12_esc_names(), "esc-names.ima");
    dumpfloppy::report_options opt{};
    opt.color = false;
    opt.hex_boot = false;
    opt.show_unused = false;
    std::ostringstream report;
    dumpfloppy::write_report(esc, report, opt);
    const std::string text = report.str();

    REQUIRE(text.find('\x1b') == std::string::npos);
    REQUIRE(text.find('\x07') == std::string::npos);
    const std::string oem = line_containing(text, "OEM");
    REQUIRE(oem.find("\\x1b]0;OEM\\x07") != std::string::npos);
    REQUIRE(oem.find("(none)") == std::string::npos);
    const std::string name = line_containing(text, "\\x1bELLO.TXT");
    REQUIRE_FALSE(name.empty());
    REQUIRE(name.find('\x1b') == std::string::npos);

    dumpfloppy::analysis cleared = esc;
    cleared.bpb.oem.clear();
    std::ostringstream none_out;
    dumpfloppy::write_report(cleared, none_out, opt);
    const std::string none = line_containing(none_out.str(), "OEM");
    REQUIRE(none.find("(none)") != std::string::npos);
    REQUIRE(none.find("\\x1b") == std::string::npos);
}

TEST_CASE("--offset escapes the owner path and a bare name_83", "[offset][escape]")
{
    dumpfloppy::analysis a =
        analyse_bytes(dumpfloppy_test::make_fat12_esc_names(), "esc-offset.ima");
    const std::string line = dumpfloppy::format_fat_offset(a, 2048);
    REQUIRE(line == "2048 sector 4 cluster 2 \\x1bELLO.TXT");
    REQUIRE(line.find('\x1b') == std::string::npos);

    dumpfloppy::dir_entry* owner = nullptr;
    for (dumpfloppy::dir_entry& e : a.entries)
    {
        if (e.path.find('\x1b') != std::string::npos)
        {
            owner = &e;
            break;
        }
    }
    REQUIRE(owner != nullptr);
    owner->path.clear();
    owner->name_83 = "\x1bX";
    const std::string bare = dumpfloppy::format_fat_offset(a, 2048);
    REQUIRE(bare == "2048 sector 4 cluster 2 \\x1bX");
    REQUIRE(bare.find('\x1b') == std::string::npos);
}

TEST_CASE("CLI --no-color escapes OEM and --offset names", "[cli][bin][escape]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch("esc-cli");
    const auto img = dir / "esc.ima";
    {
        std::ofstream out(img, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        const auto bytes = dumpfloppy_test::make_fat12_esc_names();
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
        REQUIRE(out);
    }

    int rc_report = 0;
    const std::string report = slurp_popen(std::string("\"") + bin +
                                                "\" --no-color --no-hex \"" + img.string() +
                                                "\" 2>&1",
                                            rc_report);
    REQUIRE(rc_report == 0);
    REQUIRE(report.find('\x1b') == std::string::npos);
    REQUIRE(report.find("\\x1b]0;OEM\\x07") != std::string::npos);
    REQUIRE(report.find("\\x1bELLO.TXT") != std::string::npos);

    int rc_off = 0;
    const std::string off = slurp_popen(std::string("\"") + bin +
                                             "\" --no-color --offset=2048 \"" + img.string() +
                                             "\" 2>&1",
                                         rc_off);
    REQUIRE(rc_off == 0);
    REQUIRE(off.find('\x1b') == std::string::npos);
    REQUIRE(off.find("2048 sector 4 cluster 2 \\x1bELLO.TXT") != std::string::npos);
}

TEST_CASE("FAT extract keeps subdirectories and distinct flattened names",
          "[extract][fat][subdir]")
{
    const dumpfloppy::analysis a =
        analyse_bytes(dumpfloppy_test::make_fat12_nested(), "nested.ima");
    const auto dest = scratch("fat-nested");
    std::string err;
    REQUIRE(extract_all(a, dest, err) == 4);
    REQUIRE(err.find("unsafe") == std::string::npos);

    REQUIRE(slurp_file(dest / "RAMTEST" / "MANUAL.RT") == "MANUALRT");
    REQUIRE(slurp_file(dest / "SUB" / "FILE.TXT") == "NESTED!!");
    REQUIRE(slurp_file(dest / "SUB" / "GAME.EXE") == "MZEXE!!!");
    REQUIRE(slurp_file(dest / "SUB_FILE.TXT") == "ROOTLEAF");
    REQUIRE_FALSE(std::filesystem::exists(dest / "RAMTEST_MANUAL.RT"));
    REQUIRE_FALSE(std::filesystem::exists(dest / "SUB_GAME.EXE"));
    REQUIRE(count_regular_files(dest) == 4);
}

TEST_CASE("-x '*.EXE' selects a nested file by its 8.3 name", "[extract][fat][glob]")
{
    const dumpfloppy::analysis a =
        analyse_bytes(dumpfloppy_test::make_fat12_nested(), "nested-exe.ima");
    const auto dest = scratch("fat-exe");
    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    opt.patterns.emplace_back("*.EXE");
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 1);
    REQUIRE(err.str().find("no files matched") == std::string::npos);
    REQUIRE(slurp_file(dest / "SUB" / "GAME.EXE") == "MZEXE!!!");
    REQUIRE_FALSE(std::filesystem::exists(dest / "GAME.EXE"));
    REQUIRE_FALSE(std::filesystem::exists(dest / "SUB_GAME.EXE"));
    REQUIRE_FALSE(std::filesystem::exists(dest / "SUB" / "FILE.TXT"));
    REQUIRE(count_regular_files(dest) == 1);

    /* Path and LFN are `notes`, so `*.EXE` matches only name_83. */
    dumpfloppy::analysis only{};
    dumpfloppy::dir_entry nested = payload("SUB\\notes", "GAME.EXE");
    nested.lfn = "notes";
    only.entries.push_back(nested);
    const auto dest83 = scratch("fat-exe-83");
    dumpfloppy::extract_options by_83{};
    by_83.enabled = true;
    by_83.dest_dir = dest83;
    by_83.patterns.emplace_back("*.EXE");
    std::ostringstream err83;
    REQUIRE(dumpfloppy::extract_files(only, by_83, err83) == 1);
    REQUIRE(std::filesystem::is_regular_file(dest83 / "SUB" / "notes"));
    REQUIRE_FALSE(std::filesystem::exists(dest83 / "GAME.EXE"));
    REQUIRE_FALSE(std::filesystem::exists(dest83 / "notes"));

    const char* bin = bin_or_require();
    const auto cli_dir = scratch("fat-exe-cli");
    const auto img = cli_dir / "nested.ima";
    {
        std::ofstream out(img, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        const auto bytes = dumpfloppy_test::make_fat12_nested();
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
        REQUIRE(out);
    }
    const auto cli_dest = cli_dir / "out";
    int rc = 0;
    const std::string cmd = std::string("\"") + bin + "\" -x '*.EXE' -o \"" +
                            cli_dest.string() + "\" \"" + img.string() + "\" 2>&1";
    const std::string out = slurp_popen(cmd, rc);
    REQUIRE(rc == 0);
    REQUIRE(out.find("no files matched") == std::string::npos);
    REQUIRE(slurp_file(cli_dest / "SUB" / "GAME.EXE") == "MZEXE!!!");
    REQUIRE(count_regular_files(cli_dest) == 1);
}

TEST_CASE("a FAT component longer than 255 bytes is shortened with FNV-1a",
          "[extract][fat][namemax]")
{
    const std::string long_a(256, 'A');
    const std::string short_a = std::string(240, 'A') + "_087462c5";
    dumpfloppy::analysis a{};
    a.entries.push_back(payload(long_a, "LONG.TXT"));
    const auto dest = scratch("fat-long-a");
    std::string err;
    REQUIRE(extract_all(a, dest, err) == 1);
    REQUIRE(err.find("cannot write") == std::string::npos);
    REQUIRE(std::filesystem::is_regular_file(dest / short_a));
    REQUIRE(short_a.size() == 249u);
    REQUIRE(count_regular_files(dest) == 1);

    const std::string long_c(256, 'C');
    const std::string short_c = std::string(240, 'C') + "_588a1ac5";
    dumpfloppy::analysis nested{};
    nested.entries.push_back(payload("SUB\\" + long_c + "\\FILE.TXT", "FILE.TXT"));
    const auto dest_c = scratch("fat-long-c");
    std::string err_c;
    REQUIRE(extract_all(nested, dest_c, err_c) == 1);
    REQUIRE(std::filesystem::is_regular_file(dest_c / "SUB" / short_c / "FILE.TXT"));
    REQUIRE_FALSE(std::filesystem::exists(dest_c / "SUB_FILE.TXT"));
    REQUIRE(count_regular_files(dest_c) == 1);

    std::string raw(256, 'A');
    raw[0] = '\x01';
    const std::string masked = std::string(1, '_') + std::string(239, 'A') + "_c6767adb";
    dumpfloppy::analysis hostile{};
    hostile.entries.push_back(payload(raw, "HOST.TXT"));
    const auto dest_h = scratch("fat-long-mask");
    std::string err_h;
    REQUIRE(extract_all(hostile, dest_h, err_h) == 1);
    REQUIRE(std::filesystem::is_regular_file(dest_h / masked));
    for (const auto& ent : std::filesystem::recursive_directory_iterator(dest_h))
    {
        const std::string name = ent.path().filename().string();
        REQUIRE(name.size() <= 255u);
        REQUIRE(name.find('\x01') == std::string::npos);
    }
}

TEST_CASE("a 255-byte FAT component is not shortened", "[extract][fat][namemax]")
{
    const std::string leaf(255, 'D');
    dumpfloppy::analysis a{};
    a.entries.push_back(payload(leaf, "EDGE.TXT"));
    const auto dest = scratch("fat-255");
    std::string err;
    REQUIRE(extract_all(a, dest, err) == 1);
    REQUIRE(std::filesystem::is_regular_file(dest / leaf));
    REQUIRE(leaf.size() == 255u);
}

TEST_CASE("FAT extract still rejects a .. component", "[extract][fat][safety]")
{
    dumpfloppy::analysis a{};
    a.entries.push_back(payload("SUB\\..\\outside.txt", "OUT.TXT"));
    const auto root = scratch("fat-dotdot");
    const auto dest = root / "out";
    std::string err;
    REQUIRE(extract_all(a, dest, err) == 0);
    REQUIRE(err.find("unsafe") != std::string::npos);
    REQUIRE_FALSE(std::filesystem::exists(root / "outside.txt"));
    REQUIRE_FALSE(std::filesystem::exists(dest / "outside.txt"));
    REQUIRE_FALSE(std::filesystem::exists(dest / "SUB" / "outside.txt"));
}
