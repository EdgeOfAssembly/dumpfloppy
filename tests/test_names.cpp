/**
 * @file test_names.cpp
 * @brief Empty-payload XXH64 and escaped directory names in the report.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/util.hpp"
#include "image_builder.hpp"

#include <tui/ansi.h>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace
{

std::filesystem::path write_temp(const std::vector<uint8_t>& bytes,
                                 const std::string& name)
{
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto path = dir / name;
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    REQUIRE(out);
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    REQUIRE(out);
    return path;
}

const dumpfloppy::dir_entry* find_if(
    const dumpfloppy::analysis& a,
    bool (*pred)(const dumpfloppy::dir_entry&))
{
    for (const dumpfloppy::dir_entry& e : a.entries)
    {
        if (pred(e))
        {
            return &e;
        }
    }
    return nullptr;
}

bool is_trunc(const dumpfloppy::dir_entry& e)
{
    return e.deleted && e.size == 100u && e.name_83.find("RUNC") != std::string::npos;
}

bool is_part(const dumpfloppy::dir_entry& e)
{
    return e.deleted && e.size == 600u && e.name_83.find("ART") != std::string::npos;
}

bool is_garbage(const dumpfloppy::dir_entry& e)
{
    return e.name_83.find('\xff') != std::string::npos &&
           e.name_83.find('\n') != std::string::npos;
}

bool is_zeroed(const dumpfloppy::dir_entry& e)
{
    return e.deleted && !e.name_83.empty() && e.name_83[0] == '?' &&
           e.name_83.find('\0') != std::string::npos;
}

/**
 * @brief FAT12 image with a reused deleted chain, a short chain, and bad names.
 *
 * TRUNC.BIN is deleted at HELLO.TXT's cluster (empty read, size 100).
 * PART.BIN is deleted at cluster 4 and stops before live LIVE.BIN at cluster 5.
 * One live dirent carries 0xFF and a newline. One deleted dirent is otherwise zero.
 */
std::vector<uint8_t> make_names_image()
{
    std::vector<uint8_t> img = dumpfloppy_test::make_fat12_sample();
    uint8_t* fat0 = dumpfloppy_test::fat12_fat0(img);
    const size_t fat_len = dumpfloppy_test::fat12_fat_len();
    dumpfloppy_test::fat12_chain(fat0, fat_len, 2, 2);
    dumpfloppy_test::fat12_chain(fat0, fat_len, 5, 5);
    dumpfloppy_test::fat12_mirror_fat1(img);

    uint8_t* root = dumpfloppy_test::fat12_root(img);
    dumpfloppy_test::put_file_dirent(root + 96, "TRUNC   BIN", 2, 100, true);
    dumpfloppy_test::put_file_dirent(root + 128, "PART    BIN", 4, 600, true);
    dumpfloppy_test::put_file_dirent(root + 160, "LIVE    BIN", 5, 4, false);

    uint8_t* garb = root + 192;
    dumpfloppy_test::put_file_dirent(garb, "GARBAGE TXT", 0, 0, false);
    const uint8_t raw_name[11] = {
        0xFFu, 0x0Au, 'R', 'B', 'A', 'G', 'E', ' ', 'T', 'X', 'T'};
    std::memcpy(garb, raw_name, sizeof(raw_name));

    uint8_t* zeroed = root + 224;
    dumpfloppy_test::put_file_dirent(zeroed, "ZEROED  BIN", 0, 0, true);
    std::memset(zeroed + 1, 0, 10);

    const size_t data = dumpfloppy_test::fat12_data_off();
    const size_t part_off = data + (4u - 2u) * dumpfloppy_test::k_bps;
    std::memcpy(img.data() + part_off, "PARTDATA", 8);
    return img;
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

} /* namespace */

TEST_CASE("truncated deleted file does not hash an empty buffer", "[names][xxh64]")
{
    const auto bytes = make_names_image();
    const auto img = write_temp(bytes, "names-hash.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));

    const dumpfloppy::dir_entry* trunc = find_if(a, is_trunc);
    REQUIRE(trunc != nullptr);
    REQUIRE(trunc->size == 100u);
    REQUIRE(trunc->cluster_chain.empty());
    const std::vector<uint8_t> trunc_bytes =
        dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *trunc);
    REQUIRE(trunc_bytes.empty());
    REQUIRE(trunc->xxh64.empty());
    REQUIRE(trunc->xxh64 != "ef46db3751d8e999");

    const dumpfloppy::dir_entry* part = find_if(a, is_part);
    REQUIRE(part != nullptr);
    REQUIRE(part->cluster_chain.size() == 1u);
    const std::vector<uint8_t> part_bytes =
        dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *part);
    REQUIRE_FALSE(part_bytes.empty());
    REQUIRE(part_bytes.size() < part->size);
    REQUIRE(part->xxh64 == dumpfloppy::xxh64_hex(part_bytes));
    REQUIRE(part->xxh64 != "ef46db3751d8e999");

    const dumpfloppy::dir_entry* garb = find_if(a, is_garbage);
    REQUIRE(garb != nullptr);
    REQUIRE(garb->name_83.find('\xff') != std::string::npos);
    REQUIRE(garb->name_83.find('\n') != std::string::npos);
    REQUIRE(garb->path.find('\xff') != std::string::npos);
}

TEST_CASE("report escapes 0xFF, newline, and a zeroed directory name",
          "[names][report]")
{
    const auto bytes = make_names_image();
    const auto img = write_temp(bytes, "names-report.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));

    dumpfloppy::report_options opt{};
    opt.color = true;
    opt.hex_boot = false;
    opt.show_unused = false;
    std::ostringstream colored;
    dumpfloppy::write_report(a, colored, opt);
    const std::string s = colored.str();

    REQUIRE(s.find("ef46db3751d8e999") == std::string::npos);
    REQUIRE(s.find(std::string(1, '\xff')) == std::string::npos);

    const std::string garb_line = line_containing(s, "\\xff");
    REQUIRE_FALSE(garb_line.empty());
    REQUIRE(garb_line.find("\\xff") != std::string::npos);
    REQUIRE(garb_line.find("\\x0a") != std::string::npos);
    REQUIRE(garb_line.find("RBAGE.TXT") != std::string::npos);
    REQUIRE(garb_line.find('\n') == std::string::npos);
    REQUIRE(garb_line.find(std::string(1, '\xff')) == std::string::npos);

    const std::string zero_esc =
        std::string("?\\x00\\x00\\x00\\x00\\x00\\x00\\x00.\\x00\\x00\\x00");
    const std::string zero_line = line_containing(s, "\\x00\\x00");
    REQUIRE(zero_line.find(zero_esc) != std::string::npos);
    REQUIRE(zero_line.find('\0') == std::string::npos);
    REQUIRE(zero_line.find('\n') == std::string::npos);
    REQUIRE(zero_line.find(TUI_BG_BRIGHT_RED) != std::string::npos);
    REQUIRE(zero_line.find(TUI_WHITE) != std::string::npos);
    REQUIRE(zero_line.find(TUI_BOLD) != std::string::npos);
    REQUIRE(zero_line.find(TUI_BLINK) == std::string::npos);

    const dumpfloppy::dir_entry* zeroed = find_if(a, is_zeroed);
    REQUIRE(zeroed != nullptr);
    REQUIRE(zeroed->name_83.find('\0') != std::string::npos);
}
