/**
 * @file test_review_036.cpp
 * @brief Catch2 fixtures for the 0.36 walk, extract, and forensics fixes.
 *
 * Each case fails if the 0.35 behaviour returns: a global depth or entry
 * cap, an 8192-cluster file truncate, raw slack paths, a quadratic carve,
 * an extract abort, a refused `-o` symlink, slack that stops at EOF, or a
 * slash inside one FAT name becoming a host directory.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/fat.hpp"
#include "dumpfloppy/forensics.hpp"
#include "image_builder.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <span>
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
    if (WIFEXITED(closed))
    {
        rc = WEXITSTATUS(closed);
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

dumpfloppy::analysis analyse_bytes(const std::vector<uint8_t>& bytes,
                                   const std::filesystem::path& path)
{
    write_bytes(path, bytes);
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded.has_value());
    return dumpfloppy::analyse(std::move(*loaded));
}

const dumpfloppy::dir_entry* find_name(const dumpfloppy::analysis& a, const std::string& name)
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

/**
 * @brief FAT12 or FAT16 volume of @p data_clusters, root of 16 entries.
 *
 * @p spf must cover the FAT. Cluster 2 is the first data cluster.
 */
struct volume
{
    std::vector<uint8_t> img{};
    std::size_t root_off = 0;
    std::size_t data_off = 0;
    std::size_t fat_len = 0;
    bool fat16 = false;
};

volume make_volume(bool fat16, uint32_t data_clusters, uint32_t spf)
{
    constexpr uint32_t bps = 512;
    constexpr uint32_t reserved = 1;
    constexpr uint32_t fats = 2;
    constexpr uint32_t root_entries = 16;
    constexpr uint32_t root_sectors = 1;
    const uint32_t first = reserved + fats * spf + root_sectors;
    const uint32_t total = first + data_clusters;

    volume v;
    v.fat16 = fat16;
    v.img.assign(static_cast<std::size_t>(total) * bps, 0);
    v.fat_len = static_cast<std::size_t>(spf) * bps;
    v.root_off = static_cast<std::size_t>(reserved + fats * spf) * bps;
    v.data_off = static_cast<std::size_t>(first) * bps;

    uint8_t* b = v.img.data();
    b[0] = 0xEB;
    b[1] = 0x3C;
    b[2] = 0x90;
    std::memcpy(b + 3, "DUMPFLPY", 8);
    dumpfloppy_test::poke_le16(b + 11, static_cast<uint16_t>(bps));
    b[13] = 1;
    dumpfloppy_test::poke_le16(b + 14, static_cast<uint16_t>(reserved));
    b[16] = static_cast<uint8_t>(fats);
    dumpfloppy_test::poke_le16(b + 17, static_cast<uint16_t>(root_entries));
    dumpfloppy_test::poke_le16(b + 19, static_cast<uint16_t>(total));
    b[21] = 0xF8;
    dumpfloppy_test::poke_le16(b + 22, static_cast<uint16_t>(spf));
    dumpfloppy_test::poke_le16(b + 24, 32);
    dumpfloppy_test::poke_le16(b + 26, 2);
    b[510] = 0x55;
    b[511] = 0xAA;

    std::span<uint8_t> fat{v.img.data() + static_cast<std::size_t>(reserved) * bps, v.fat_len};
    const auto kind = fat16 ? dumpfloppy::fat_kind::fat16 : dumpfloppy::fat_kind::fat12;
    REQUIRE(dumpfloppy::fat_set(fat, kind, 0, fat16 ? 0xFFF8 : 0x0FF8));
    REQUIRE(dumpfloppy::fat_set(fat, kind, 1, 0xFFFF));
    std::memcpy(fat.data() + v.fat_len, fat.data(), v.fat_len);
    return v;
}

void mirror_fat(volume& v)
{
    std::memcpy(v.img.data() + v.root_off - v.fat_len, v.img.data() + (v.root_off - 2u * v.fat_len),
                v.fat_len);
}

std::span<uint8_t> fat0_of(volume& v)
{
    return std::span<uint8_t>(v.img.data() + (v.root_off - 2u * v.fat_len), v.fat_len);
}

void chain_eof(volume& v, uint32_t cluster)
{
    const auto kind = v.fat16 ? dumpfloppy::fat_kind::fat16 : dumpfloppy::fat_kind::fat12;
    auto fat = fat0_of(v);
    REQUIRE(dumpfloppy::fat_set(fat, kind, cluster, 0xFFFF));
    mirror_fat(v);
}

void chain_to(volume& v, uint32_t from, uint32_t to)
{
    const auto kind = v.fat16 ? dumpfloppy::fat_kind::fat16 : dumpfloppy::fat_kind::fat12;
    auto fat = fat0_of(v);
    REQUIRE(dumpfloppy::fat_set(fat, kind, from, static_cast<uint16_t>(to)));
}

uint8_t lfn_checksum(const uint8_t name[11])
{
    uint8_t sum = 0;
    for (int i = 0; i < 11; ++i)
    {
        const uint8_t rotated =
            static_cast<uint8_t>(((sum & 1u) != 0u ? 0x80u : 0u) + (sum >> 1));
        sum = static_cast<uint8_t>(rotated + name[i]);
    }
    return sum;
}

void poke_utf16(uint8_t* p, uint16_t unit)
{
    p[0] = static_cast<uint8_t>(unit & 0xFFu);
    p[1] = static_cast<uint8_t>((unit >> 8) & 0xFFu);
}

/**
 * @brief Write LFN slots then the 8.3 entry. Returns slots consumed.
 *
 * Slots are stored highest sequence first, which is what the parser prepends.
 */
std::size_t put_lfn_dir(uint8_t* dest, const std::string& lfn, const char* name11,
                        uint16_t cluster, bool is_dir)
{
    uint8_t short_name[11];
    std::memset(short_name, ' ', 11);
    const std::size_t n11 = std::strlen(name11);
    std::memcpy(short_name, name11, n11 > 11u ? 11u : n11);
    const uint8_t sum = lfn_checksum(short_name);
    const std::size_t nslots = (lfn.size() + 12u) / 13u;
    for (std::size_t seq = nslots; seq >= 1u; --seq)
    {
        uint8_t* slot = dest + (nslots - seq) * 32u;
        std::memset(slot, 0, 32);
        slot[0] = static_cast<uint8_t>(seq | (seq == nslots ? 0x40u : 0u));
        slot[11] = 0x0F;
        slot[13] = sum;
        const std::size_t base = (seq - 1u) * 13u;
        bool ended = false;
        for (std::size_t c = 0; c < 13u; ++c)
        {
            const std::size_t idx = base + c;
            uint16_t unit = 0xFFFFu;
            if (!ended && idx < lfn.size())
            {
                unit = static_cast<uint8_t>(lfn[idx]);
            }
            else if (!ended)
            {
                unit = 0;
                ended = true;
            }
            const std::size_t off = (c < 5u) ? (1u + c * 2u) : (c < 11u) ? (14u + (c - 5u) * 2u)
                                                                          : (28u + (c - 11u) * 2u);
            poke_utf16(slot + off, unit);
        }
        if (seq == 1u)
        {
            break;
        }
    }
    uint8_t* ent = dest + nslots * 32u;
    if (is_dir)
    {
        dumpfloppy_test::put_dir_dirent(ent, name11, cluster);
    }
    else
    {
        dumpfloppy_test::put_file_dirent(ent, name11, cluster, 1);
    }
    return nslots + 1u;
}

} /* namespace */

TEST_CASE("depth cap skips one subtree and still lists SECRET.TXT", "[review036][directory]")
{
    const char* bin = bin_or_require();
    auto v = make_volume(false, 40u, 1u);
    for (uint32_t c = 2; c <= 34u; ++c)
    {
        chain_eof(v, c);
    }
    dumpfloppy_test::put_dir_dirent(v.img.data() + v.root_off, "D00        ", 2);
    dumpfloppy_test::put_file_dirent(v.img.data() + v.root_off + 32u, "SECRET  TXT", 34, 6);
    std::memcpy(v.img.data() + v.data_off + 32u * 512u, "SECRET", 6);
    for (uint32_t level = 0; level < 31u; ++level)
    {
        uint8_t* slot = v.img.data() + v.data_off + static_cast<std::size_t>(level) * 512u;
        char name[12];
        std::snprintf(name, sizeof(name), "D%02u        ", static_cast<unsigned>(level + 1u));
        dumpfloppy_test::put_dir_dirent(slot, name, static_cast<uint16_t>(3u + level));
    }
    uint8_t* hidden =
        v.img.data() + v.data_off + 31u * 512u;
    dumpfloppy_test::put_file_dirent(hidden, "HIDDEN  TXT", 0, 0);

    const auto dir = scratch_root("depth-secret");
    const auto img = dir / "depth.img";
    const dumpfloppy::analysis a = analyse_bytes(v.img, img);
    REQUIRE(find_name(a, "SECRET.TXT") != nullptr);
    REQUIRE(a.directory_cap_depth);
    REQUIRE_FALSE(a.directory_cap_entries);

    const auto out = dir / "out";
    int rc = 0;
    const std::string err =
        slurp_popen(std::string("\"") + bin + "\" -x -o \"" + out.string() + "\" \"" +
                        img.string() + "\" 2>&1 >/dev/null",
                    rc);
    REQUIRE(std::filesystem::is_regular_file(out / "SECRET.TXT"));
    REQUIRE(std::filesystem::file_size(out / "SECRET.TXT") == 6u);
    REQUIRE(rc == 0);
    REQUIRE(err.find("dumpfloppy:") != std::string::npos);
    REQUIRE(err.find("depth") != std::string::npos);
    REQUIRE(err.find(img.filename().string()) != std::string::npos);
}

TEST_CASE("entry cap keeps a later root file ZZZ.TXT", "[review036][directory]")
{
    const char* bin = bin_or_require();
    constexpr uint32_t dir_clusters = 282;
    auto v = make_volume(false, dir_clusters + 2u, 2u);
    const uint32_t dir_last = 2u + dir_clusters - 1u;
    for (uint32_t c = 2; c < dir_last; ++c)
    {
        chain_to(v, c, c + 1u);
    }
    chain_eof(v, dir_last);
    chain_eof(v, dir_last + 1u);
    mirror_fat(v);
    dumpfloppy_test::put_dir_dirent(v.img.data() + v.root_off, "MANY       ", 2);
    dumpfloppy_test::put_file_dirent(v.img.data() + v.root_off + 32u, "ZZZ     TXT",
                                     static_cast<uint16_t>(dir_last + 1u), 3);
    std::memcpy(v.img.data() + v.data_off + static_cast<std::size_t>(dir_clusters) * 512u, "ZZZ",
                3);
    for (uint32_t c = 0; c < dir_clusters; ++c)
    {
        uint8_t* cluster = v.img.data() + v.data_off + static_cast<std::size_t>(c) * 512u;
        for (int slot = 0; slot < 16; ++slot)
        {
            char name[12];
            std::snprintf(name, sizeof(name), "F%05u  TXT",
                          static_cast<unsigned>(c) * 16u + static_cast<unsigned>(slot));
            dumpfloppy_test::put_file_dirent(cluster + static_cast<std::size_t>(slot) * 32u, name,
                                             0, 0);
        }
    }

    const auto dir = scratch_root("entries-zzz");
    const auto img = dir / "many.img";
    const dumpfloppy::analysis a = analyse_bytes(v.img, img);
    REQUIRE(find_name(a, "ZZZ.TXT") != nullptr);
    REQUIRE(a.directory_cap_entries);
    REQUIRE(a.entries.size() > dumpfloppy::k_max_dir_entries);

    const auto out = dir / "out";
    int rc = 0;
    const std::string err =
        slurp_popen(std::string("\"") + bin + "\" -x -o \"" + out.string() + "\" \"" +
                        img.string() + "\" 2>&1 >/dev/null",
                    rc);
    REQUIRE(std::filesystem::is_regular_file(out / "ZZZ.TXT"));
    REQUIRE(std::filesystem::file_size(out / "ZZZ.TXT") == 3u);
    REQUIRE(rc == 0);
    REQUIRE(err.find("dumpfloppy:") != std::string::npos);
    REQUIRE(err.find("entries") != std::string::npos);
    REQUIRE(err.find(img.filename().string()) != std::string::npos);
}

TEST_CASE("directory chain cap does not hide a later root file", "[review036][directory]")
{
    const char* bin = bin_or_require();
    constexpr uint32_t dir_clusters = 8193;
    auto v = make_volume(true, dir_clusters + 1u, 33u);
    const uint32_t dir_last = 2u + dir_clusters - 1u;
    for (uint32_t c = 2; c < dir_last; ++c)
    {
        chain_to(v, c, c + 1u);
    }
    chain_eof(v, dir_last);
    chain_eof(v, dir_last + 1u);
    mirror_fat(v);
    dumpfloppy_test::put_dir_dirent(v.img.data() + v.root_off, "LONGDIR    ", 2);
    dumpfloppy_test::put_file_dirent(v.img.data() + v.root_off + 32u, "ZZZ     TXT",
                                     static_cast<uint16_t>(dir_last + 1u), 3);
    std::memcpy(v.img.data() + v.data_off + static_cast<std::size_t>(dir_clusters) * 512u, "ZZZ",
                3);

    const auto dir = scratch_root("chain-cap-local");
    const auto img = dir / "longdir.img";
    const dumpfloppy::analysis a = analyse_bytes(v.img, img);
    REQUIRE(find_name(a, "ZZZ.TXT") != nullptr);
    REQUIRE(a.directory_cap_chain);
    const dumpfloppy::dir_entry* longdir = find_name(a, "LONGDIR");
    REQUIRE(longdir != nullptr);
    REQUIRE(longdir->cluster_chain.size() ==
            static_cast<std::size_t>(dumpfloppy::k_max_chain_steps));
    REQUIRE(longdir->notes.find("chain capped") != std::string::npos);

    int rc = 0;
    const std::string err =
        slurp_popen(std::string("\"") + bin + "\" --no-color --no-hex \"" + img.string() +
                        "\" 2>&1 >/dev/null",
                    rc);
    REQUIRE(rc == 0);
    REQUIRE(err.find("dumpfloppy:") != std::string::npos);
    REQUIRE(err.find("chain") != std::string::npos);
    REQUIRE(err.find(img.filename().string()) != std::string::npos);
}

TEST_CASE("slack path escapes ESC and chain tails are slack", "[review036][slack]")
{
    auto sample = dumpfloppy_test::make_fat12_sample();
    uint8_t* hello = dumpfloppy_test::fat12_root(sample) + 32;
    hello[0] = 0x1B;
    const auto dir = scratch_root("slack-esc");
    const auto img = dir / "esc.img";
    const dumpfloppy::analysis esc = analyse_bytes(sample, img);
    dumpfloppy::forensics_request req;
    req.slack = true;
    std::ostringstream out;
    std::ostringstream err;
    REQUIRE(dumpfloppy::write_forensics(esc, req, out, err) == 0);
    const std::string text = out.str();
    REQUIRE(text.find('\x1b') == std::string::npos);
    REQUIRE(text.find("\\x1b") != std::string::npos);

    auto v = make_volume(false, 8u, 1u);
    chain_to(v, 2, 3);
    chain_to(v, 3, 4);
    chain_eof(v, 4);
    chain_eof(v, 5);
    mirror_fat(v);
    dumpfloppy_test::put_file_dirent(v.img.data() + v.root_off, "LONG    TXT", 2, 10);
    dumpfloppy_test::put_file_dirent(v.img.data() + v.root_off + 32u, "ZERO    TXT", 5, 0);
    std::memcpy(v.img.data() + v.data_off, "0123456789", 10);
    std::memcpy(v.img.data() + v.data_off + 512u, "HIDDEN SECRET TEXT", 19);
    std::memcpy(v.img.data() + v.data_off + 2u * 512u, "TAIL SECRET TEXT!!", 18);
    dumpfloppy_test::put_file_dirent(v.img.data() + v.data_off + 3u * 512u, "LEAKME  TXT", 0, 4);
    std::memcpy(v.img.data() + v.data_off + 3u * 512u + 32u, "ZEROCLUSTERSECRET", 17);

    const dumpfloppy::analysis a = analyse_bytes(v.img, dir / "tail.img");
    const dumpfloppy::dir_entry* longer = find_name(a, "LONG.TXT");
    const dumpfloppy::dir_entry* zero = find_name(a, "ZERO.TXT");
    REQUIRE(longer != nullptr);
    REQUIRE(zero != nullptr);
    REQUIRE(longer->notes.find("chain longer than size") != std::string::npos);
    REQUIRE(zero->notes.find("chain longer than size") != std::string::npos);
    REQUIRE(longer->cluster_chain.size() == 3u);

    req.slack = true;
    req.carve = true;
    req.leaked = true;
    std::ostringstream slack_out;
    std::ostringstream slack_err;
    REQUIRE(dumpfloppy::write_forensics(a, req, slack_out, slack_err) == 0);
    const std::string body = slack_out.str();
    const auto hex_of = [](const char* s)
    {
        static constexpr char k_hex[] = "0123456789abcdef";
        std::string h;
        for (const char* p = s; *p != '\0'; ++p)
        {
            const auto c = static_cast<unsigned char>(*p);
            h.push_back(k_hex[c >> 4]);
            h.push_back(k_hex[c & 0x0Fu]);
        }
        return h;
    };
    REQUIRE(body.find(hex_of("HIDDEN SECRET TEXT")) != std::string::npos);
    REQUIRE(body.find(hex_of("ZEROCLUSTERSECRET")) != std::string::npos);
    REQUIRE(body.find("HIDDEN SECRET TEXT") != std::string::npos);
    REQUIRE(body.find("LEAKME") != std::string::npos);
}

TEST_CASE("carve merges directory spans and stays under 3 seconds", "[review036][carve]")
{
    constexpr uint32_t dir_clusters = 320;
    constexpr uint32_t free_clusters = 4096;
    auto v = make_volume(true, dir_clusters + free_clusters, 18u);
    const uint32_t dir_last = 2u + dir_clusters - 1u;
    for (uint32_t c = 2; c < dir_last; ++c)
    {
        chain_to(v, c, c + 1u);
    }
    chain_eof(v, dir_last);
    mirror_fat(v);
    dumpfloppy_test::put_dir_dirent(v.img.data() + v.root_off, "DIRS       ", 2);
    v.img[v.data_off] = static_cast<uint8_t>('M');
    v.img[v.data_off + 1u] = static_cast<uint8_t>('Z');
    const std::size_t free_off = v.data_off + static_cast<std::size_t>(dir_clusters) * 512u;
    v.img[free_off] = static_cast<uint8_t>('M');
    v.img[free_off + 1u] = static_cast<uint8_t>('Z');

    const auto dir = scratch_root("carve-merge");
    const dumpfloppy::analysis a = analyse_bytes(v.img, dir / "carve.img");
    dumpfloppy::forensics_request req;
    req.carve = true;
    std::ostringstream out;
    std::ostringstream err;
    const auto t0 = std::chrono::steady_clock::now();
    REQUIRE(dumpfloppy::write_forensics(a, req, out, err) == 0);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - t0)
                        .count();
    REQUIRE(ms < 3000);
    const std::string text = out.str();
    REQUIRE(text.find(std::to_string(free_off) + " MZ") != std::string::npos);
    REQUIRE(text.find(std::to_string(v.data_off) + " MZ") == std::string::npos);
}

TEST_CASE("one cannot-create does not drop a later root file", "[review036][extract]")
{
    const char* bin = bin_or_require();
    auto v = make_volume(false, 8u, 1u);
    chain_eof(v, 2);
    chain_eof(v, 3);
    chain_eof(v, 4);
    chain_eof(v, 5);
    dumpfloppy_test::put_file_dirent(v.img.data() + v.root_off, "X          ", 2, 1);
    dumpfloppy_test::put_dir_dirent(v.img.data() + v.root_off + 32u, "X          ", 3);
    dumpfloppy_test::put_file_dirent(v.img.data() + v.root_off + 64u, "Z       TXT", 5, 1);
    v.img[v.data_off] = static_cast<uint8_t>('X');
    dumpfloppy_test::put_file_dirent(v.img.data() + v.data_off + 512u, "Y       TXT", 4, 1);
    v.img[v.data_off + 2u * 512u] = static_cast<uint8_t>('Y');
    v.img[v.data_off + 3u * 512u] = static_cast<uint8_t>('Z');

    const auto dir = scratch_root("clash-xz");
    const auto img = dir / "clash.img";
    write_bytes(img, v.img);
    const auto out = dir / "out";
    int rc = 0;
    const std::string err =
        slurp_popen(std::string("\"") + bin + "\" -x -o \"" + out.string() + "\" \"" +
                        img.string() + "\" 2>&1 >/dev/null",
                    rc);
    REQUIRE(rc != 0);
    REQUIRE(err.find("cannot create") != std::string::npos);
    REQUIRE(std::filesystem::is_regular_file(out / "X"));
    REQUIRE(std::filesystem::is_regular_file(out / "Z.TXT"));
    REQUIRE_FALSE(std::filesystem::exists(out / "X" / "Y.TXT"));
}

TEST_CASE("a path past PATH_MAX skips that entry and still writes ZLATER.TXT",
          "[review036][extract]")
{
    const char* bin = bin_or_require();
    constexpr int levels = 26;
    auto v = make_volume(false, static_cast<uint32_t>(levels) + 4u, 1u);
    for (int i = 0; i < levels + 2; ++i)
    {
        chain_eof(v, static_cast<uint32_t>(2 + i));
    }
    std::string deep(180, 'A');
    deep[0] = 'L';
    deep[1] = '0';
    const std::size_t root_slots =
        put_lfn_dir(v.img.data() + v.root_off, deep, "D00        ", 2, true);
    dumpfloppy_test::put_file_dirent(v.img.data() + v.root_off + root_slots * 32u, "ZLATER  TXT",
                                     static_cast<uint16_t>(2 + levels + 1), 1);
    v.img[v.data_off + static_cast<std::size_t>(levels + 1) * 512u] = static_cast<uint8_t>('Z');
    for (int level = 0; level < levels - 1; ++level)
    {
        std::string name(180, 'B');
        name[0] = 'L';
        name[1] = static_cast<char>('A' + (level % 26));
        name[2] = static_cast<char>('0' + (level % 10));
        char short_name[12];
        std::snprintf(short_name, sizeof(short_name), "D%02d        ", level + 1);
        uint8_t* cluster = v.img.data() + v.data_off + static_cast<std::size_t>(level) * 512u;
        put_lfn_dir(cluster, name, short_name, static_cast<uint16_t>(3 + level), true);
    }
    uint8_t* bottom =
        v.img.data() + v.data_off + static_cast<std::size_t>(levels - 1) * 512u;
    dumpfloppy_test::put_file_dirent(bottom, "DEEP    TXT", static_cast<uint16_t>(2 + levels), 1);
    v.img[v.data_off + static_cast<std::size_t>(levels) * 512u] = static_cast<uint8_t>('D');

    const auto dir = scratch_root("path-max");
    const auto img = dir / "long.img";
    write_bytes(img, v.img);
    const auto out = dir / "out";
    int rc = 0;
    const std::string err =
        slurp_popen(std::string("\"") + bin + "\" -x -o \"" + out.string() + "\" \"" +
                        img.string() + "\" 2>&1 >/dev/null",
                    rc);
    REQUIRE(rc != 0);
    REQUIRE(err.find("cannot create") != std::string::npos);
    REQUIRE(std::filesystem::is_regular_file(out / "ZLATER.TXT"));
    REQUIRE_FALSE(std::filesystem::exists(out / "DEEP.TXT"));
}

TEST_CASE("a symlinked -o directory is the extract destination", "[review036][extract]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("symlink-dest");
    const auto img = dir / "sample.ima";
    write_bytes(img, dumpfloppy_test::make_fat12_sample());
    const auto real = dir / "real";
    const auto link = dir / "link";
    std::filesystem::create_directories(real);
    std::filesystem::create_directory_symlink(real, link);

    int rc = 0;
    const std::string err =
        slurp_popen(std::string("\"") + bin + "\" -x -o \"" + link.string() + "\" \"" +
                        img.string() + "\" 2>&1 >/dev/null",
                    rc);
    REQUIRE(rc == 0);
    REQUIRE(err.find("skip symlink path") == std::string::npos);
    REQUIRE(std::filesystem::is_regular_file(real / "HELLO.TXT"));
    REQUIRE(std::filesystem::file_size(real / "HELLO.TXT") == 14u);
}

TEST_CASE("a slash inside an 8.3 name is one host component", "[review036][extract]")
{
    const char* bin = bin_or_require();
    auto v = make_volume(false, 4u, 1u);
    chain_eof(v, 2);
    chain_eof(v, 3);
    dumpfloppy_test::put_file_dirent(v.img.data() + v.root_off, "A/B     TXT", 2, 1);
    dumpfloppy_test::put_file_dirent(v.img.data() + v.root_off + 32u, "C\\D     TXT", 3, 1);
    v.img[v.data_off] = static_cast<uint8_t>('A');
    v.img[v.data_off + 512u] = static_cast<uint8_t>('C');

    const auto dir = scratch_root("slash-name");
    const auto img = dir / "slash.img";
    write_bytes(img, v.img);
    const auto out = dir / "out";
    int rc = 0;
    slurp_popen(std::string("\"") + bin + "\" -x -o \"" + out.string() + "\" \"" + img.string() +
                    "\" 2>/dev/null",
                rc);
    REQUIRE(rc == 0);
    REQUIRE(std::filesystem::is_regular_file(out / "A_B.TXT"));
    REQUIRE(std::filesystem::is_regular_file(out / "C_D.TXT"));
    REQUIRE_FALSE(std::filesystem::is_directory(out / "A"));
    REQUIRE_FALSE(std::filesystem::is_directory(out / "C"));
    REQUIRE_FALSE(std::filesystem::exists(out / "A" / "B.TXT"));
}
