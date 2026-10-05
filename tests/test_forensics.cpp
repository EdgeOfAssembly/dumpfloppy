/**
 * @file test_forensics.cpp
 * @brief Symlink-safe extract, directory-walk caps, and FAT slack/leaked/carve/sources.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/cli.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/fat.hpp"
#include "dumpfloppy/fat_slack.h"
#include "dumpfloppy/forensics.hpp"
#include "dumpfloppy/version.hpp"
#include "image_builder.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <vector>

namespace
{

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

dumpfloppy::cli_options parse(const std::vector<std::string>& args)
{
    std::vector<std::string> storage;
    storage.reserve(args.size() + 1u);
    storage.emplace_back("dumpfloppy");
    storage.insert(storage.end(), args.begin(), args.end());
    std::vector<char*> ptrs;
    ptrs.reserve(storage.size());
    for (std::string& s : storage)
    {
        ptrs.push_back(s.data());
    }
    return dumpfloppy::parse_cli(static_cast<int>(ptrs.size()), ptrs.data());
}

std::filesystem::path scratch_root(const char* name)
{
    const auto dir =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / name;
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir;
}

void write_bytes(const std::filesystem::path& path, const std::vector<uint8_t>& bytes)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    REQUIRE(out);
    if (!bytes.empty())
    {
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }
    REQUIRE(out);
}

dumpfloppy::analysis analyse_file(const std::filesystem::path& path)
{
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded.has_value());
    return dumpfloppy::analyse(std::move(*loaded));
}

void require_chains_bounded(const dumpfloppy::analysis& a)
{
    const std::size_t file_limit = static_cast<std::size_t>(
        a.fat.max_cluster == 0u ? 1u : a.fat.max_cluster);
    for (const dumpfloppy::dir_entry& e : a.entries)
    {
        const bool is_dir =
            (e.attributes & dumpfloppy::k_attr_directory) != 0u;
        const std::size_t limit =
            is_dir ? static_cast<std::size_t>(dumpfloppy::k_max_chain_steps) : file_limit;
        REQUIRE(e.cluster_chain.size() <= limit);
    }
}

/**
 * @brief FAT12 whose subdirectory clusters point at each other.
 *
 * Root SUB → cluster 2. Clusters 2, 3, and 4 each hold 16 directory slots
 * aimed at the next cluster. Cluster 5 aims back at 2. A walk that records
 * only the first cluster of a subdirectory reparses the loop until the
 * entry cap. Recording every parsed cluster keeps the listing small.
 */
std::vector<uint8_t> make_multiply_dirs()
{
    using dumpfloppy_test::k_bps;
    using dumpfloppy_test::k_total_sec;

    std::vector<uint8_t> img(static_cast<std::size_t>(k_total_sec) * k_bps, 0);
    dumpfloppy_test::write_min_fat12_boot(img.data());
    uint8_t* fat0 = dumpfloppy_test::fat12_fat0(img);
    const std::size_t fat_len = dumpfloppy_test::fat12_fat_len();
    dumpfloppy_test::fat12_init_media(fat0, fat_len);
    for (uint32_t c = 2; c <= 5u; ++c)
    {
        REQUIRE(fat12_entry_set(fat0, fat_len, c, 0x0FFFu) == 0);
    }
    dumpfloppy_test::fat12_mirror_fat1(img);

    dumpfloppy_test::put_dir_dirent(dumpfloppy_test::fat12_root(img), "SUB        ", 2);

    const std::size_t data = dumpfloppy_test::fat12_data_off();
    const uint16_t next_of[4] = {3, 4, 5, 2};
    for (uint32_t n = 0; n < 4u; ++n)
    {
        uint8_t* cluster = img.data() + data + static_cast<std::size_t>(n) * k_bps;
        for (int slot = 0; slot < 16; ++slot)
        {
            char name11[12];
            name11[0] = 'D';
            name11[1] = static_cast<char>('2' + static_cast<int>(n));
            name11[2] = static_cast<char>('A' + slot);
            for (int k = 3; k < 11; ++k)
            {
                name11[k] = ' ';
            }
            name11[11] = '\0';
            dumpfloppy_test::put_dir_dirent(cluster + static_cast<std::size_t>(slot) * 32u,
                                            name11, next_of[n]);
        }
    }
    return img;
}

/** @brief One subdirectory of 256 clusters × 16 files (past the entry cap). */
std::vector<uint8_t> make_entry_cap_image()
{
    constexpr uint32_t bps = 512;
    constexpr uint32_t spf = 1;
    constexpr uint32_t reserved = 1;
    constexpr uint32_t fats = 2;
    constexpr uint32_t root_sectors = 1;
    constexpr uint32_t dir_clusters = 256;
    constexpr uint32_t data_clusters = 260;
    constexpr uint32_t first = reserved + fats * spf + root_sectors;
    constexpr uint32_t total = first + data_clusters;

    std::vector<uint8_t> img(static_cast<std::size_t>(total) * bps, 0);
    uint8_t* b = img.data();
    b[0] = 0xEB;
    b[1] = 0x3C;
    b[2] = 0x90;
    std::memcpy(b + 3, "DUMPFLPY", 8);
    dumpfloppy_test::poke_le16(b + 11, static_cast<uint16_t>(bps));
    b[13] = 1;
    dumpfloppy_test::poke_le16(b + 14, static_cast<uint16_t>(reserved));
    b[16] = static_cast<uint8_t>(fats);
    dumpfloppy_test::poke_le16(b + 17, 16);
    dumpfloppy_test::poke_le16(b + 19, static_cast<uint16_t>(total));
    b[21] = 0xF8;
    dumpfloppy_test::poke_le16(b + 22, static_cast<uint16_t>(spf));
    dumpfloppy_test::poke_le16(b + 24, 32);
    dumpfloppy_test::poke_le16(b + 26, 2);
    b[510] = 0x55;
    b[511] = 0xAA;

    uint8_t* fat0 = img.data() + static_cast<std::size_t>(reserved) * bps;
    const std::size_t fat_len = static_cast<std::size_t>(spf) * bps;
    REQUIRE(fat12_entry_set(fat0, fat_len, 0, 0xFF8u) == 0);
    REQUIRE(fat12_entry_set(fat0, fat_len, 1, 0xFFFu) == 0);
    const uint32_t dir_last = 2u + dir_clusters - 1u;
    for (uint32_t c = 2; c < dir_last; ++c)
    {
        REQUIRE(fat12_entry_set(fat0, fat_len, c, static_cast<uint16_t>(c + 1u)) == 0);
    }
    REQUIRE(fat12_entry_set(fat0, fat_len, dir_last, 0xFFFu) == 0);
    std::memcpy(fat0 + fat_len, fat0, fat_len);

    const std::size_t root_off = static_cast<std::size_t>(reserved + fats * spf) * bps;
    dumpfloppy_test::put_dir_dirent(img.data() + root_off, "SUB        ", 2);

    const std::size_t data = static_cast<std::size_t>(first) * bps;
    for (uint32_t c = 0; c < dir_clusters; ++c)
    {
        uint8_t* cluster = img.data() + data + static_cast<std::size_t>(c) * bps;
        for (int slot = 0; slot < 16; ++slot)
        {
            dumpfloppy_test::put_file_dirent(cluster + static_cast<std::size_t>(slot) * 32u,
                                             "FILE    TXT", 0, 0);
        }
    }
    return img;
}

/**
 * @brief FAT16 file whose cluster chain is longer than @ref k_max_chain_steps.
 *
 * 9000 data clusters, one file chained from cluster 2 through 9001.
 * The size field is the full chain so extract must write every byte.
 */
std::vector<uint8_t> make_chain_cap_image()
{
    constexpr uint32_t bps = 512;
    constexpr uint32_t spf = 36;
    constexpr uint32_t reserved = 1;
    constexpr uint32_t fats = 2;
    constexpr uint32_t root_sectors = 1;
    constexpr uint32_t data_clusters = 9000;
    constexpr uint32_t first = reserved + fats * spf + root_sectors;
    constexpr uint32_t total = first + data_clusters;

    std::vector<uint8_t> img(static_cast<std::size_t>(total) * bps, 0);
    uint8_t* b = img.data();
    b[0] = 0xEB;
    b[1] = 0x3C;
    b[2] = 0x90;
    std::memcpy(b + 3, "DUMPFLPY", 8);
    dumpfloppy_test::poke_le16(b + 11, static_cast<uint16_t>(bps));
    b[13] = 1;
    dumpfloppy_test::poke_le16(b + 14, static_cast<uint16_t>(reserved));
    b[16] = static_cast<uint8_t>(fats);
    dumpfloppy_test::poke_le16(b + 17, 16);
    dumpfloppy_test::poke_le16(b + 19, static_cast<uint16_t>(total));
    b[21] = 0xF8;
    dumpfloppy_test::poke_le16(b + 22, static_cast<uint16_t>(spf));
    dumpfloppy_test::poke_le16(b + 24, 63);
    dumpfloppy_test::poke_le16(b + 26, 16);
    b[510] = 0x55;
    b[511] = 0xAA;

    uint8_t* fat0 = img.data() + static_cast<std::size_t>(reserved) * bps;
    const std::size_t fat_len = static_cast<std::size_t>(spf) * bps;
    std::span<uint8_t> fat{fat0, fat_len};
    REQUIRE(dumpfloppy::fat_set(fat, dumpfloppy::fat_kind::fat16, 0, 0xFFF8));
    REQUIRE(dumpfloppy::fat_set(fat, dumpfloppy::fat_kind::fat16, 1, 0xFFFF));
    const uint32_t last = 1u + data_clusters;
    for (uint32_t c = 2; c < last; ++c)
    {
        REQUIRE(dumpfloppy::fat_set(fat, dumpfloppy::fat_kind::fat16, c,
                                    static_cast<uint16_t>(c + 1u)));
    }
    REQUIRE(dumpfloppy::fat_set(fat, dumpfloppy::fat_kind::fat16, last, 0xFFFF));
    std::memcpy(fat0 + fat_len, fat0, fat_len);

    const std::size_t root_off = static_cast<std::size_t>(reserved + fats * spf) * bps;
    const uint32_t full = data_clusters * bps;
    dumpfloppy_test::put_file_dirent(img.data() + root_off, "BIG     BIN", 2, full);
    return img;
}

/** @brief Sample image plus an MZ slack tail and a free-cluster dirent. */
std::vector<uint8_t> make_planted_sample()
{
    std::vector<uint8_t> img = dumpfloppy_test::make_fat12_sample();
    const std::size_t data = dumpfloppy_test::fat12_data_off();
    REQUIRE(data == 2048u);
    img[data + 14u] = static_cast<uint8_t>('M');
    img[data + 15u] = static_cast<uint8_t>('Z');
    const std::size_t leaked = data + 2u * dumpfloppy_test::k_bps;
    REQUIRE(leaked == 3072u);
    dumpfloppy_test::put_file_dirent(img.data() + leaked, "LEAKED  TXT", 0, 12);
    return img;
}

/**
 * @brief Slot-aligned dirent in HELLO slack, plus an unaligned one at slack start.
 *
 * HELLO.TXT is 14 bytes in a 512-byte cluster, so slack begins at cluster+14.
 * SLOT32.TXT is planted at cluster+32 and lies entirely in that tail.
 * LIVE2.TXT is another 14-byte file; OFF14.TXT is planted at its slack start
 * and is not a directory slot.
 */
std::vector<uint8_t> make_slack_slot_image()
{
    std::vector<uint8_t> img = dumpfloppy_test::make_fat12_sample();
    uint8_t* fat0 = dumpfloppy_test::fat12_fat0(img);
    const std::size_t fat_len = dumpfloppy_test::fat12_fat_len();
    REQUIRE(fat12_entry_set(fat0, fat_len, 4u, 0x0FFFu) == 0);
    dumpfloppy_test::fat12_mirror_fat1(img);

    dumpfloppy_test::put_file_dirent(dumpfloppy_test::fat12_root(img) + 96, "LIVE2   TXT", 4,
                                     14);

    const std::size_t data = dumpfloppy_test::fat12_data_off();
    REQUIRE(data == 2048u);
    dumpfloppy_test::put_file_dirent(img.data() + data + 32u, "SLOT32  TXT", 0, 4);

    const std::size_t live2 = data + 2u * dumpfloppy_test::k_bps;
    REQUIRE(live2 == 3072u);
    std::memset(img.data() + live2, static_cast<int>('A'), 14);
    dumpfloppy_test::put_file_dirent(img.data() + live2 + 14u, "OFF14   TXT", 0, 4);
    return img;
}

/**
 * @brief Directory SUBA chains cluster 2 into 3. SUBB also starts at cluster 3.
 *
 * Cluster 3 holds ONLYC3.TXT and nothing else does. A visited set that records
 * every parsed directory cluster lists that name once. A set that records only
 * SUBA's first cluster parses cluster 3 again under SUBB.
 */
std::vector<uint8_t> make_shared_cluster_dirs()
{
    using dumpfloppy_test::k_bps;
    using dumpfloppy_test::k_total_sec;

    std::vector<uint8_t> img(static_cast<std::size_t>(k_total_sec) * k_bps, 0);
    dumpfloppy_test::write_min_fat12_boot(img.data());
    uint8_t* fat0 = dumpfloppy_test::fat12_fat0(img);
    const std::size_t fat_len = dumpfloppy_test::fat12_fat_len();
    dumpfloppy_test::fat12_init_media(fat0, fat_len);
    REQUIRE(fat12_entry_set(fat0, fat_len, 2u, 3u) == 0);
    REQUIRE(fat12_entry_set(fat0, fat_len, 3u, 0x0FFFu) == 0);
    dumpfloppy_test::fat12_mirror_fat1(img);

    uint8_t* root = dumpfloppy_test::fat12_root(img);
    dumpfloppy_test::put_dir_dirent(root, "SUBA", 2);
    dumpfloppy_test::put_dir_dirent(root + 32, "SUBB", 3);

    const std::size_t data = dumpfloppy_test::fat12_data_off();
    dumpfloppy_test::put_file_dirent(img.data() + data + k_bps, "ONLYC3  TXT", 0, 0);
    return img;
}

std::size_t count_substr(const std::string& hay, const std::string& needle)
{
    std::size_t count = 0;
    std::size_t pos = 0;
    while ((pos = hay.find(needle, pos)) != std::string::npos)
    {
        ++count;
        pos += needle.size();
    }
    return count;
}

std::string section_from(const std::string& text, const char* header)
{
    const auto at = text.find(header);
    if (at == std::string::npos)
    {
        return {};
    }
    return text.substr(at);
}

void plant_text(std::vector<uint8_t>& img, std::size_t off, const char* text)
{
    const std::size_t n = std::strlen(text);
    REQUIRE(off + n <= img.size());
    std::memcpy(img.data() + off, text, n);
}

/**
 * @brief `#include` in the live file and again in slack, plus a case and a straddle.
 *
 * Cluster 3 is freed so the bytes after the slack tail are their own region.
 * `#include` planted across that boundary must not match.
 */
std::vector<uint8_t> make_source_include_image()
{
    std::vector<uint8_t> img = dumpfloppy_test::make_fat12_sample();
    uint8_t* fat0 = dumpfloppy_test::fat12_fat0(img);
    const std::size_t fat_len = dumpfloppy_test::fat12_fat_len();
    REQUIRE(fat12_entry_set(fat0, fat_len, 3u, 0u) == 0);
    dumpfloppy_test::fat12_mirror_fat1(img);

    const std::size_t data = dumpfloppy_test::fat12_data_off();
    REQUIRE(data == 2048u);
    plant_text(img, data, "#include");
    plant_text(img, data + 14u, "#include");
    plant_text(img, data + 22u, "#INCLUDE");
    plant_text(img, data + 512u - 4u, "#include");
    return img;
}

/**
 * @brief Needles in free clusters and past the filesystem, plus BASIC lines.
 */
std::vector<uint8_t> make_source_pattern_image()
{
    std::vector<uint8_t> img = dumpfloppy_test::make_fat12_sample();
    const std::size_t data = dumpfloppy_test::fat12_data_off();
    REQUIRE(data == 2048u);
    plant_text(img, data + 2u * dumpfloppy_test::k_bps, "proc near");
    plant_text(img, data + 2u * dumpfloppy_test::k_bps + 9u, "#INCLUDE");
    plant_text(img, data + 3u * dumpfloppy_test::k_bps, "org 100h");
    plant_text(img, data + 4u * dumpfloppy_test::k_bps, "\n10 PRINT \"HI\"\nx10 PRINT\n");
    const std::size_t long_at = data + 5u * dumpfloppy_test::k_bps;
    plant_text(img, long_at, "10 ");
    std::memset(img.data() + long_at + 3u, static_cast<int>('A'), 60u);
    plant_text(img, data + 7u * dumpfloppy_test::k_bps, "\r20 GOTO");
    const std::size_t volume_end = img.size();
    REQUIRE(volume_end == 64u * dumpfloppy_test::k_bps);
    img.resize(volume_end + 16u);
    plant_text(img, volume_end, "uses crt");
    return img;
}

/** @brief 65 copies of `#include` in HELLO slack (tail plus the next chain cluster). */
std::vector<uint8_t> make_source_cap_image()
{
    std::vector<uint8_t> img = dumpfloppy_test::make_fat12_sample();
    uint8_t* fat0 = dumpfloppy_test::fat12_fat0(img);
    const std::size_t fat_len = dumpfloppy_test::fat12_fat_len();
    REQUIRE(fat12_entry_set(fat0, fat_len, 2u, 3u) == 0);
    dumpfloppy_test::fat12_mirror_fat1(img);

    const std::size_t data = dumpfloppy_test::fat12_data_off();
    REQUIRE(data == 2048u);
    const std::size_t slack = data + 14u;
    for (int i = 0; i < 62; ++i)
    {
        plant_text(img, slack + static_cast<std::size_t>(i) * 8u, "#include");
    }
    const std::size_t cluster3 = data + dumpfloppy_test::k_bps;
    for (int i = 0; i < 3; ++i)
    {
        plant_text(img, cluster3 + static_cast<std::size_t>(i) * 8u, "#include");
    }
    return img;
}

const dumpfloppy::dir_entry* find_name(const dumpfloppy::analysis& a,
                                       const std::string& name)
{
    for (const dumpfloppy::dir_entry& e : a.entries)
    {
        if (e.name_83 == name || e.path == name)
        {
            return &e;
        }
    }
    return nullptr;
}

} /* namespace */

TEST_CASE("fat_slack_bytes covers zero, multiple, and remainder", "[forensics][slack]")
{
    REQUIRE(fat_slack_bytes(0u, 512u) == 0u);
    REQUIRE(fat_slack_bytes(100u, 0u) == 0u);
    REQUIRE(fat_slack_bytes(512u, 512u) == 0u);
    REQUIRE(fat_slack_bytes(500u, 512u) == 12u);
    REQUIRE(fat_slack_bytes(14u, 512u) == 498u);
}

TEST_CASE("extract skips a symlink final component and leaves the target absent",
          "[forensics][extract][symlink]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("symlink-final");
    const auto img = dir / "sample.ima";
    write_bytes(img, dumpfloppy_test::make_fat12_sample());
    const auto dest = dir / "out";
    std::filesystem::create_directories(dest);
    const auto absent = dir / "absent-hello-target";
    std::filesystem::create_symlink(absent, dest / "HELLO.TXT");

    int rc = 0;
    const std::string cmd = std::string("\"") + bin + "\" -x -o \"" + dest.string() +
                            "\" \"" + img.string() + "\" 2>&1";
    const std::string out = slurp_popen(cmd, rc);
    REQUIRE(rc == 0);
    const std::string hello = (dest / "HELLO.TXT").string();
    REQUIRE(out.find("dumpfloppy: skip symlink path '" + hello + "'") != std::string::npos);
    REQUIRE(std::filesystem::is_symlink(dest / "HELLO.TXT"));
    REQUIRE(std::filesystem::read_symlink(dest / "HELLO.TXT") == absent);
    REQUIRE_FALSE(std::filesystem::exists(absent));
    REQUIRE(std::filesystem::is_regular_file(dest / "?ONE.TXT"));
    REQUIRE(std::filesystem::file_size(dest / "?ONE.TXT") == 4u);
}

TEST_CASE("extract skips when an ancestor of the dest is a symlink",
          "[forensics][extract][symlink]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("symlink-ancestor");
    const auto img = dir / "sample.ima";
    write_bytes(img, dumpfloppy_test::make_fat12_sample());
    const auto absent = dir / "absent-tree";
    const auto via = dir / "via";
    std::filesystem::create_symlink(absent, via);
    const auto dest = via / "out";

    int rc = 0;
    const std::string cmd = std::string("\"") + bin + "\" -x -o \"" + dest.string() +
                            "\" \"" + img.string() + "\" 2>&1";
    const std::string out = slurp_popen(cmd, rc);
    REQUIRE(rc == 0);
    const std::string hello = (dest / "HELLO.TXT").string();
    REQUIRE(out.find("dumpfloppy: skip symlink path '" + hello + "'") != std::string::npos);
    REQUIRE(std::filesystem::is_symlink(via));
    REQUIRE(std::filesystem::read_symlink(via) == absent);
    REQUIRE_FALSE(std::filesystem::exists(absent));
    REQUIRE_FALSE(std::filesystem::exists(absent / "out"));
    REQUIRE_FALSE(std::filesystem::exists(absent / "out" / "HELLO.TXT"));
    REQUIRE_FALSE(std::filesystem::exists(absent / "HELLO.TXT"));
}

TEST_CASE("multiplying directory clusters stay under the entry cap",
          "[forensics][directory]")
{
    const auto dir = scratch_root("dir-multiply");
    const auto img = dir / "multiply.ima";
    write_bytes(img, make_multiply_dirs());
    const dumpfloppy::analysis a = analyse_file(img);

    REQUIRE(a.kind == dumpfloppy::fat_kind::fat12);
    REQUIRE(a.bpb.looks_valid);
    REQUIRE_FALSE(a.directory_capped);
    REQUIRE(a.entries.size() < 1000u);
    REQUIRE(a.entries.size() >= 60u);
    require_chains_bounded(a);
}

TEST_CASE("visited set keeps a shared later directory cluster once",
          "[forensics][directory]")
{
    const auto dir = scratch_root("dir-shared-cluster");
    const auto img = dir / "shared.ima";
    write_bytes(img, make_shared_cluster_dirs());
    const dumpfloppy::analysis a = analyse_file(img);

    REQUIRE(a.kind == dumpfloppy::fat_kind::fat12);
    REQUIRE(a.bpb.looks_valid);
    REQUIRE_FALSE(a.directory_capped);
    require_chains_bounded(a);

    const dumpfloppy::dir_entry* suba = find_name(a, "SUBA");
    const dumpfloppy::dir_entry* subb = find_name(a, "SUBB");
    REQUIRE(suba != nullptr);
    REQUIRE(subb != nullptr);
    REQUIRE(suba->cluster_chain.size() == 2u);
    REQUIRE(suba->cluster_chain[0] == 2u);
    REQUIRE(suba->cluster_chain[1] == 3u);
    REQUIRE(subb->first_cluster == 3u);
    REQUIRE(subb->cluster_chain.size() == 1u);
    REQUIRE(subb->cluster_chain[0] == 3u);

    std::size_t n = 0;
    for (const dumpfloppy::dir_entry& e : a.entries)
    {
        if (e.name_83 != "ONLYC3.TXT")
        {
            continue;
        }
        ++n;
        REQUIRE(e.path == "SUBA\\ONLYC3.TXT");
    }
    REQUIRE(n == 1u);
}

TEST_CASE("directory entry cap stops at 4096 and the image still exits 0",
          "[forensics][directory]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("dir-entry-cap");
    const auto img = dir / "entries.ima";
    write_bytes(img, make_entry_cap_image());
    const dumpfloppy::analysis a = analyse_file(img);

    REQUIRE(a.kind == dumpfloppy::fat_kind::fat12);
    REQUIRE(a.directory_capped);
    REQUIRE(a.entries.size() == 4096u);
    require_chains_bounded(a);

    int rc = 0;
    const std::string err =
        slurp_popen(std::string("\"") + bin + "\" --no-color \"" + img.string() +
                        "\" 2>&1 >/dev/null",
                    rc);
    REQUIRE(rc == 0);
    REQUIRE(err.find("dumpfloppy:") != std::string::npos);
    REQUIRE(err.find(img.filename().string()) != std::string::npos);
    REQUIRE(err.find("entries") != std::string::npos);
    REQUIRE(err.find("Warning: directory walk stopped at cap") == std::string::npos);
    REQUIRE(a.directory_cap_entries);
    REQUIRE_FALSE(a.directory_cap_depth);
}

TEST_CASE("FAT16 file of 9000 clusters extracts in full", "[forensics][directory]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("dir-chain-cap");
    const auto img = dir / "chain.ima";
    write_bytes(img, make_chain_cap_image());
    const dumpfloppy::analysis a = analyse_file(img);

    REQUIRE(a.kind == dumpfloppy::fat_kind::fat16);
    REQUIRE_FALSE(a.directory_capped);
    const dumpfloppy::dir_entry* big = find_name(a, "BIG.BIN");
    REQUIRE(big != nullptr);
    REQUIRE(big->cluster_chain.size() == 9000u);
    REQUIRE(big->notes.find("chain capped") == std::string::npos);
    require_chains_bounded(a);

    const auto out = dir / "out";
    int rc = 0;
    const std::string cmd = std::string("\"") + bin + "\" -x -o \"" + out.string() +
                            "\" \"" + img.string() + "\" 2>/dev/null";
    const std::string text = slurp_popen(cmd, rc);
    (void)text;
    REQUIRE(rc == 0);
    REQUIRE(std::filesystem::file_size(out / "BIG.BIN") == 9000u * 512u);
}

TEST_CASE("slack tail, leaked free-cluster dirent, and MZ carve",
          "[forensics][slack][leaked][carve]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("forensics-plant");
    const auto img = dir / "plant.ima";
    write_bytes(img, make_planted_sample());
    const dumpfloppy::analysis a = analyse_file(img);

    REQUIRE(a.kind == dumpfloppy::fat_kind::fat12);
    REQUIRE(find_name(a, "HELLO.TXT") != nullptr);
    REQUIRE(find_name(a, "LEAKED.TXT") == nullptr);
    REQUIRE(find_name(a, "LEAKED  TXT") == nullptr);

    dumpfloppy::forensics_request req;
    req.slack = true;
    req.leaked = true;
    req.carve = true;
    std::ostringstream out;
    std::ostringstream err;
    REQUIRE(dumpfloppy::write_forensics(a, req, out, err) == 0);
    const std::string text = out.str();
    REQUIRE(err.str().empty());
    REQUIRE(text.find("=== Slack ===\n") != std::string::npos);
    REQUIRE(text.find("HELLO.TXT 2062 498 4d5a") != std::string::npos);
    REQUIRE(text.find("=== Leaked directory entries ===\n") != std::string::npos);
    REQUIRE(text.find("3072 'LEAKED.TXT' 20 0 12\n") != std::string::npos);
    REQUIRE(text.find("=== Carve ===\n") != std::string::npos);
    REQUIRE(text.find("2062 MZ 2 MZ\n") != std::string::npos);

    int rc = 0;
    const std::string cli =
        slurp_popen(std::string("\"") + bin +
                        "\" --no-color --no-hex --slack --leaked --carve \"" +
                        img.string() + "\" 2>&1",
                    rc);
    REQUIRE(rc == 0);
    const auto dir_at = cli.find("DIRECTORY");
    const auto shown_at = cli.find("entries shown", dir_at);
    REQUIRE(dir_at != std::string::npos);
    REQUIRE(shown_at != std::string::npos);
    const std::string directory = cli.substr(dir_at, shown_at - dir_at);
    REQUIRE(directory.find("LEAKED") == std::string::npos);
    REQUIRE(directory.find("HELLO.TXT") != std::string::npos);
    REQUIRE(cli.find("HELLO.TXT 2062 498 4d5a") != std::string::npos);
    REQUIRE(cli.find("3072 'LEAKED.TXT' 20 0 12") != std::string::npos);
    REQUIRE(cli.find("2062 MZ 2 MZ") != std::string::npos);

    int plain_rc = 0;
    const std::string plain =
        slurp_popen(std::string("\"") + bin + "\" --no-color --no-hex \"" + img.string() +
                        "\"",
                    plain_rc);
    REQUIRE(plain_rc == 0);
    REQUIRE(plain.find("=== Slack ===") == std::string::npos);
    REQUIRE(plain.find("=== Leaked directory entries ===") == std::string::npos);
    REQUIRE(plain.find("=== Carve ===") == std::string::npos);
    const auto plain_dir = plain.find("DIRECTORY");
    const auto plain_shown = plain.find("entries shown", plain_dir);
    REQUIRE(plain_dir != std::string::npos);
    REQUIRE(plain_shown != std::string::npos);
    REQUIRE(plain.substr(plain_dir, plain_shown - plain_dir).find("LEAKED") ==
            std::string::npos);
    REQUIRE(plain.find("HELLO.TXT") != std::string::npos);
}

TEST_CASE("leaked slack dirent is reported only on a cluster-aligned slot",
          "[forensics][leaked]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("leaked-slack-align");
    const auto img = dir / "align.ima";
    write_bytes(img, make_slack_slot_image());
    const dumpfloppy::analysis a = analyse_file(img);

    REQUIRE(find_name(a, "HELLO.TXT") != nullptr);
    REQUIRE(find_name(a, "LIVE2.TXT") != nullptr);
    REQUIRE(find_name(a, "SLOT32.TXT") == nullptr);
    REQUIRE(find_name(a, "OFF14.TXT") == nullptr);

    dumpfloppy::forensics_request req;
    req.leaked = true;
    std::ostringstream out;
    std::ostringstream err;
    REQUIRE(dumpfloppy::write_forensics(a, req, out, err) == 0);
    const std::string text = out.str();
    REQUIRE(err.str().empty());
    REQUIRE(text.find("=== Leaked directory entries ===\n") != std::string::npos);
    REQUIRE(text.find("2080 'SLOT32.TXT' 20 0 4\n") != std::string::npos);
    REQUIRE(text.find("3086") == std::string::npos);
    REQUIRE(text.find("OFF14") == std::string::npos);

    int rc = 0;
    const std::string cli =
        slurp_popen(std::string("\"") + bin + "\" --no-color --no-hex --leaked \"" +
                        img.string() + "\" 2>&1",
                    rc);
    REQUIRE(rc == 0);
    REQUIRE(cli.find("2080 'SLOT32.TXT' 20 0 4") != std::string::npos);
    const auto leaked_at = cli.find("=== Leaked directory entries ===");
    REQUIRE(leaked_at != std::string::npos);
    const std::string leaked = cli.substr(leaked_at);
    REQUIRE(leaked.find("3086") == std::string::npos);
    REQUIRE(leaked.find("OFF14") == std::string::npos);
    REQUIRE(cli.find("OFF14") == std::string::npos);
}

TEST_CASE("help documents enable-only slack leaked carve", "[forensics][cli]")
{
    const std::string usage = dumpfloppy::usage_text();
    REQUIRE(usage.find("--slack") != std::string::npos);
    REQUIRE(usage.find("--leaked") != std::string::npos);
    REQUIRE(usage.find("--carve") != std::string::npos);
    REQUIRE(usage.find("--no-slack") == std::string::npos);
    REQUIRE(usage.find("--no-leaked") == std::string::npos);
    REQUIRE(usage.find("--no-carve") == std::string::npos);

    const auto on = parse({"disk.ima", "--carve", "--slack", "--leaked"});
    REQUIRE(on.ok);
    REQUIRE(on.forensics.slack);
    REQUIRE(on.forensics.leaked);
    REQUIRE(on.forensics.carve);
    REQUIRE(on.inputs.size() == 1u);

    const auto off = parse({"disk.ima"});
    REQUIRE(off.ok);
    REQUIRE_FALSE(off.forensics.slack);
    REQUIRE_FALSE(off.forensics.leaked);
    REQUIRE_FALSE(off.forensics.carve);

    const auto twin = parse({"--no-slack", "disk.ima"});
    REQUIRE_FALSE(twin.ok);
    REQUIRE(twin.error.find("unknown option") != std::string::npos);

    const char* bin = bin_or_require();
    int rc = 0;
    const std::string help = slurp_popen(std::string("\"") + bin + "\" -h", rc);
    REQUIRE(rc == 0);
    REQUIRE(help.find("--slack") != std::string::npos);
    REQUIRE(help.find("--leaked") != std::string::npos);
    REQUIRE(help.find("--carve") != std::string::npos);
    REQUIRE(help.find("--no-slack") == std::string::npos);
    REQUIRE(help.find("--no-leaked") == std::string::npos);
    REQUIRE(help.find("--no-carve") == std::string::npos);
    REQUIRE(help.find("directory walk hit cap") != std::string::npos);
    REQUIRE(help.find("depth") != std::string::npos);
    REQUIRE(help.find("entries") != std::string::npos);
    REQUIRE(help.find("chain") != std::string::npos);
}

TEST_CASE("slack leaked carve reject a non-FAT file like offset", "[forensics][cli]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("forensics-not-fat");
    const auto txt = dir / "notes.txt";
    {
        std::ofstream out(txt, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out << "not a floppy\n";
    }

    int rc = 0;
    const std::string out =
        slurp_popen(std::string("\"") + bin +
                        "\" --slack --leaked --carve \"" + txt.string() + "\" 2>&1",
                    rc);
    REQUIRE(rc == 1);
    REQUIRE(out.find("dumpfloppy: slack is only implemented for FAT12/FAT16\n") !=
            std::string::npos);
    REQUIRE(out.find("dumpfloppy: leaked is only implemented for FAT12/FAT16\n") !=
            std::string::npos);
    REQUIRE(out.find("dumpfloppy: carve is only implemented for FAT12/FAT16\n") !=
            std::string::npos);
    REQUIRE(out.find("=== Slack ===") == std::string::npos);
    REQUIRE(out.find("HELLO") == std::string::npos);
    REQUIRE(out.find("floppy image secrets") != std::string::npos);

    int only = 0;
    const std::string slack_only =
        slurp_popen(std::string("\"") + bin + "\" --slack \"" + txt.string() + "\" 2>&1",
                    only);
    REQUIRE(only == 1);
    REQUIRE(slack_only.find("dumpfloppy: slack is only implemented for FAT12/FAT16\n") !=
            std::string::npos);
    REQUIRE(slack_only.find("leaked is only implemented") == std::string::npos);
    REQUIRE(slack_only.find("carve is only implemented") == std::string::npos);
    REQUIRE(slack_only.find("floppy image secrets") != std::string::npos);
}

TEST_CASE("sources keeps slack leaked and carve text and adds its own section",
          "[forensics][sources]")
{
    const auto dir = scratch_root("sources-prefix");
    const auto img = dir / "plant.ima";
    write_bytes(img, make_planted_sample());
    const dumpfloppy::analysis a = analyse_file(img);

    dumpfloppy::forensics_request base;
    base.slack = true;
    base.leaked = true;
    base.carve = true;
    std::ostringstream plain_out;
    std::ostringstream plain_err;
    REQUIRE(dumpfloppy::write_forensics(a, base, plain_out, plain_err) == 0);
    REQUIRE(plain_err.str().empty());

    dumpfloppy::forensics_request with = base;
    with.sources = true;
    std::ostringstream src_out;
    std::ostringstream src_err;
    REQUIRE(dumpfloppy::write_forensics(a, with, src_out, src_err) == 0);
    REQUIRE(src_err.str().empty());
    const std::string plain = plain_out.str();
    const std::string both = src_out.str();
    REQUIRE(both.rfind(plain, 0) == 0);
    REQUIRE(both.substr(plain.size()) == "=== Source ===\n");
}

TEST_CASE("sources reports slack include and ignores the live copy",
          "[forensics][sources]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("sources-include");
    const auto img = dir / "include.ima";
    write_bytes(img, make_source_include_image());
    const dumpfloppy::analysis a = analyse_file(img);
    REQUIRE(a.kind == dumpfloppy::fat_kind::fat12);

    dumpfloppy::forensics_request req;
    req.sources = true;
    std::ostringstream out;
    std::ostringstream err;
    REQUIRE(dumpfloppy::write_forensics(a, req, out, err) == 0);
    REQUIRE(err.str().empty());
    const std::string text = out.str();
    REQUIRE(text == "=== Source ===\n2062 include #include\n");
    REQUIRE(text.find("2048 ") == std::string::npos);
    REQUIRE(text.find("2556 ") == std::string::npos);
    REQUIRE(text.find("INCLUDE") == std::string::npos);

    int rc = 0;
    const std::string cli =
        slurp_popen(std::string("\"") + bin + "\" --no-color --no-hex --sources \"" +
                        img.string() + "\" 2>&1",
                    rc);
    REQUIRE(rc == 0);
    const std::string src = section_from(cli, "=== Source ===\n");
    REQUIRE(src.find("2062 include #include\n") != std::string::npos);
    REQUIRE(src.find("2048 ") == std::string::npos);
    REQUIRE(src.find("2556 ") == std::string::npos);
    REQUIRE(src.find("INCLUDE") == std::string::npos);
    REQUIRE(cli.find("=== Slack ===") == std::string::npos);
    REQUIRE(cli.find("=== Carve ===") == std::string::npos);
    REQUIRE(cli.find("=== Leaked directory entries ===") == std::string::npos);
}

TEST_CASE("sources reports free-space and past-end needles and BASIC lines",
          "[forensics][sources]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("sources-patterns");
    const auto img = dir / "patterns.ima";
    write_bytes(img, make_source_pattern_image());
    const dumpfloppy::analysis a = analyse_file(img);

    dumpfloppy::forensics_request req;
    req.sources = true;
    std::ostringstream out;
    std::ostringstream err;
    REQUIRE(dumpfloppy::write_forensics(a, req, out, err) == 0);
    REQUIRE(err.str().empty());

    const std::string long_basic = std::string("10 ") + std::string(45u, 'A');
    REQUIRE(long_basic.size() == 48u);
    const std::string expect =
        "=== Source ===\n"
        "3072 proc-near proc near\n"
        "3584 org-100h org 100h\n"
        "4097 basic 10 PRINT \"HI\"\n"
        "4608 basic " +
        long_basic +
        "\n"
        "5633 basic 20 GOTO\n"
        "32768 uses-crt uses crt\n";
    REQUIRE(out.str() == expect);
    REQUIRE(out.str().find("x10") == std::string::npos);
    REQUIRE(out.str().find("INCLUDE") == std::string::npos);

    int rc = 0;
    const std::string cli =
        slurp_popen(std::string("\"") + bin + "\" --no-color --no-hex --sources \"" +
                        img.string() + "\" 2>&1",
                    rc);
    REQUIRE(rc == 0);
    const std::string src = section_from(cli, "=== Source ===\n");
    REQUIRE(src == expect);
}

TEST_CASE("slack without sources does not print a source section", "[forensics][sources]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("sources-slack-only");
    const auto img = dir / "plant.ima";
    write_bytes(img, make_planted_sample());

    int rc = 0;
    const std::string cli =
        slurp_popen(std::string("\"") + bin + "\" --no-color --no-hex --slack \"" +
                        img.string() + "\" 2>&1",
                    rc);
    REQUIRE(rc == 0);
    REQUIRE(cli.find("=== Slack ===") != std::string::npos);
    REQUIRE(cli.find("HELLO.TXT 2062 498 4d5a") != std::string::npos);
    REQUIRE(cli.find("=== Source ===") == std::string::npos);
}

TEST_CASE("sources on a non-FAT image still lists and returns 1", "[forensics][sources]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("sources-not-fat");
    const auto txt = dir / "notes.txt";
    {
        std::ofstream file(txt, std::ios::binary | std::ios::trunc);
        REQUIRE(file);
        file << "not a floppy\n";
    }

    const dumpfloppy::analysis a = analyse_file(txt);
    dumpfloppy::forensics_request req;
    req.sources = true;
    std::ostringstream out;
    std::ostringstream err;
    REQUIRE(dumpfloppy::write_forensics(a, req, out, err) == 1);
    REQUIRE(out.str().empty());
    REQUIRE(err.str() == "dumpfloppy: sources is only implemented for FAT12/FAT16\n");

    int rc = 0;
    const std::string cli =
        slurp_popen(std::string("\"") + bin + "\" --sources \"" + txt.string() + "\" 2>&1",
                    rc);
    REQUIRE(rc == 1);
    REQUIRE(cli.find("dumpfloppy: sources is only implemented for FAT12/FAT16\n") !=
            std::string::npos);
    REQUIRE(cli.find("FAT12/FAT16") != std::string::npos);
    REQUIRE(cli.find("=== Source ===") == std::string::npos);
    REQUIRE(cli.find("floppy image secrets") != std::string::npos);
    REQUIRE(cli.find("slack is only implemented") == std::string::npos);
    REQUIRE(cli.find("leaked is only implemented") == std::string::npos);
    REQUIRE(cli.find("carve is only implemented") == std::string::npos);
}

TEST_CASE("sources stops at 64 include hits", "[forensics][sources][cap]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("sources-cap");
    const auto img = dir / "cap.ima";
    write_bytes(img, make_source_cap_image());
    const dumpfloppy::analysis a = analyse_file(img);

    dumpfloppy::forensics_request req;
    req.sources = true;
    std::ostringstream out;
    std::ostringstream err;
    REQUIRE(dumpfloppy::write_forensics(a, req, out, err) == 0);
    REQUIRE(err.str() == "Warning: source scan stopped at cap\n");
    const std::string text = out.str();
    REQUIRE(count_substr(text, " include #include\n") == 64u);
    REQUIRE(text.find("2062 include #include\n") != std::string::npos);
    REQUIRE(text.find("2568 include #include\n") != std::string::npos);
    REQUIRE(text.find("2576 include #include\n") == std::string::npos);
    REQUIRE(text.find("=== Slack ===") == std::string::npos);

    int rc = 0;
    const std::string cli =
        slurp_popen(std::string("\"") + bin + "\" --no-color --no-hex --sources \"" +
                        img.string() + "\" 2>&1",
                    rc);
    REQUIRE(rc == 0);
    REQUIRE(cli.find("Warning: source scan stopped at cap") != std::string::npos);
    const std::string src = section_from(cli, "=== Source ===\n");
    REQUIRE(count_substr(src, " include #include\n") == 64u);
    REQUIRE(src.find("2576 include #include\n") == std::string::npos);
}

TEST_CASE("help and version document enable-only sources", "[forensics][sources][cli]")
{
    const std::string usage = dumpfloppy::usage_text();
    REQUIRE(usage.find("--sources") != std::string::npos);
    REQUIRE(usage.find("--no-sources") == std::string::npos);
    REQUIRE(usage.find(dumpfloppy::k_version) != std::string::npos);
    REQUIRE(std::string(dumpfloppy::k_version) == "0.38");

    const auto on = parse({"disk.ima", "--sources"});
    REQUIRE(on.ok);
    REQUIRE(on.forensics.sources);
    REQUIRE_FALSE(on.forensics.slack);
    REQUIRE_FALSE(on.forensics.leaked);
    REQUIRE_FALSE(on.forensics.carve);
    REQUIRE(on.inputs.size() == 1u);

    const auto flipped = parse({"--sources", "disk.ima"});
    REQUIRE(flipped.ok);
    REQUIRE(flipped.forensics.sources);
    REQUIRE(flipped.inputs.size() == 1u);

    const auto off = parse({"disk.ima"});
    REQUIRE(off.ok);
    REQUIRE_FALSE(off.forensics.sources);

    const auto twin = parse({"--no-sources", "disk.ima"});
    REQUIRE_FALSE(twin.ok);
    REQUIRE(twin.error.find("unknown option") != std::string::npos);

    const char* bin = bin_or_require();
    int help_rc = 0;
    const std::string help = slurp_popen(std::string("\"") + bin + "\" --help", help_rc);
    REQUIRE(help_rc == 0);
    REQUIRE(help.find("--sources") != std::string::npos);
    REQUIRE(help.find("--no-sources") == std::string::npos);

    int ver_rc = 0;
    const std::string ver = slurp_popen(std::string("\"") + bin + "\" -v", ver_rc);
    REQUIRE(ver_rc == 0);
    REQUIRE(ver == "dumpfloppy 0.38\n");

    int bare_rc = 0;
    const std::string bare = slurp_popen(std::string("\"") + bin + "\"", bare_rc);
    REQUIRE(bare_rc == 0);
    REQUIRE(bare.find("Usage:") != std::string::npos);
    REQUIRE(bare.find("--sources") != std::string::npos);

    int none_rc = 0;
    const std::string none =
        slurp_popen(std::string("\"") + bin + "\" --sources 2>&1", none_rc);
    REQUIRE(none_rc == 1);
    REQUIRE(none.find("dumpfloppy: no image files given\n") != std::string::npos);
    REQUIRE(none.find("=== Source ===") == std::string::npos);
}
