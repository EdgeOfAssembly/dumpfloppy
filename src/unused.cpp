/**
 * @file unused.cpp
 * @brief Recover leftover bytes from FAT-free and FAT-bad clusters.
 *
 * Live files and deleted dirents with a still-allocated chain are listed
 * elsewhere. This walk is the unallocated map: format-fill (`0xF6`) is
 * ignored; consecutive dirty free clusters are one run. Typical cause:
 * files deleted without a full format, then a duplicator copies every
 * sector (Sierra FormMaster / any raw track copy). SQ2 Disk 1 is one
 * published example (AGI C sources + AGI.EXE memory map in FAT-free
 * space); the scanner is not game-specific.
 */
#include "dumpfloppy/unused.hpp"
#include "dumpfloppy/fat.hpp"
#include "dumpfloppy/fat12_codec.h"
#include "dumpfloppy/util.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <string>
#include <string_view>

namespace dumpfloppy
{
namespace
{

constexpr unsigned k_min_dirty = 8u;
constexpr unsigned k_preview_chars = 48u;

[[nodiscard]] uint32_t cluster_bytes(const bpb_info& bpb) noexcept
{
    return static_cast<uint32_t>(bpb.bytes_per_sector) *
           static_cast<uint32_t>(bpb.sectors_per_cluster);
}

[[nodiscard]] bool is_free_entry(fat_kind kind, uint16_t v) noexcept
{
    if (kind == fat_kind::fat12)
    {
        return fat12_is_free(v) != 0;
    }
    return v == 0u;
}

[[nodiscard]] bool is_bad_entry(fat_kind kind, uint16_t v) noexcept
{
    if (kind == fat_kind::fat12)
    {
        return fat12_is_bad(v) != 0;
    }
    return v == 0xFFF7u;
}

[[nodiscard]] bool cluster_has_leftover(std::span<const uint8_t> bytes) noexcept
{
    if (bytes.size() < k_min_dirty)
    {
        return false;
    }
    unsigned dirty = 0;
    const uint8_t first = bytes[0];
    bool all_same = true;
    for (uint8_t b : bytes)
    {
        if (b != first)
        {
            all_same = false;
        }
        if (b != 0u && b != 0xF6u)
        {
            ++dirty;
        }
    }
    if (all_same && (first == 0u || first == 0xF6u || first == 0xE5u ||
                     first == 0xFFu))
    {
        return false;
    }
    return dirty >= k_min_dirty;
}

[[nodiscard]] bool contains_bytes(std::span<const uint8_t> hay,
                                  std::string_view needle) noexcept
{
    if (needle.empty() || hay.size() < needle.size())
    {
        return false;
    }
    const auto* p = reinterpret_cast<const char*>(hay.data());
    return std::search(p, p + hay.size(), needle.begin(), needle.end()) !=
           p + hay.size();
}

[[nodiscard]] bool mostly_text(std::span<const uint8_t> bytes) noexcept
{
    if (bytes.empty())
    {
        return false;
    }
    unsigned ok = 0;
    for (uint8_t b : bytes)
    {
        if (b == 0x09u || b == 0x0Au || b == 0x0Du ||
            (b >= 0x20u && b <= 0x7Eu))
        {
            ++ok;
        }
    }
    return (static_cast<uint64_t>(ok) * 100u) >=
           (static_cast<uint64_t>(bytes.size()) * 85u);
}

[[nodiscard]] unsigned count_seg_off(std::span<const uint8_t> bytes) noexcept
{
    unsigned hits = 0;
    const size_t n = bytes.size();
    for (size_t i = 0; i + 3u < n; ++i)
    {
        const auto hex = [](uint8_t c) -> bool {
            return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
                   (c >= 'a' && c <= 'f');
        };
        if (bytes[i] != ':')
        {
            continue;
        }
        if (i == 0u || !hex(bytes[i - 1u]) || !hex(bytes[i + 1u]))
        {
            continue;
        }
        ++hits;
        if (hits >= 64u)
        {
            break;
        }
    }
    return hits;
}

[[nodiscard]] bool looks_like_map(std::span<const uint8_t> bytes) noexcept
{
    if (contains_bytes(bytes, "Segment ") && contains_bytes(bytes, "Addr ="))
    {
        return true;
    }
    return count_seg_off(bytes) >= 8u && mostly_text(bytes);
}

void classify(unused_run& run)
{
    const std::span<const uint8_t> p = run.payload;
    if (contains_bytes(p, "#include") || contains_bytes(p, "STRPTR"))
    {
        run.guess = "C source";
        return;
    }
    if (looks_like_map(p))
    {
        run.guess = "memory map";
        return;
    }
    if (mostly_text(p))
    {
        run.guess = "text";
        return;
    }
    run.guess = "binary";
}

void set_host_name(unused_run& run)
{
    const char* ext = ".bin";
    if (run.guess == "C source")
    {
        ext = ".c";
    }
    else if (run.guess == "memory map")
    {
        ext = ".map";
    }
    else if (run.guess == "text")
    {
        ext = ".txt";
    }
    char buf[40] = {};
    std::snprintf(buf, sizeof(buf), "unused_c%04u%s",
                  static_cast<unsigned>(run.first_cluster), ext);
    run.host_name = buf;
}

void set_preview(unused_run& run)
{
    std::string out;
    out.reserve(k_preview_chars);
    for (uint8_t b : run.payload)
    {
        if (out.size() >= k_preview_chars)
        {
            break;
        }
        if (b == 0u)
        {
            continue;
        }
        if (b == '\n' || b == '\r' || b == '\t')
        {
            if (!out.empty() && out.back() != ' ')
            {
                out.push_back(' ');
            }
            continue;
        }
        if (b >= 0x20u && b <= 0x7Eu)
        {
            out.push_back(static_cast<char>(b));
        }
    }
    while (!out.empty() && out.back() == ' ')
    {
        out.pop_back();
    }
    run.preview = std::move(out);
}

void trim_trailing_nuls(std::vector<uint8_t>& p)
{
    while (!p.empty() && p.back() == 0u)
    {
        p.pop_back();
    }
}

void emit_run(unused_run run, std::vector<unused_run>& out)
{
    if (run.payload.size() < k_min_dirty)
    {
        return;
    }
    classify(run);
    set_host_name(run);
    set_preview(run);
    run.xxh64 = xxh64_hex(run.payload);
    out.push_back(std::move(run));
}

/**
 * Split a leftover run when a linker map (seg:off columns) is followed by
 * C sources (`#include`). Any FAT12/16 image can hold that mix in one
 * free-cluster span; emit two recovered files.
 */
void finish_run(unused_run& run, uint32_t clusz, std::vector<unused_run>& out)
{
    if (run.payload.empty())
    {
        return;
    }
    trim_trailing_nuls(run.payload);
    if (run.payload.size() < k_min_dirty)
    {
        return;
    }

    const char inc[] = "#include";
    const auto* p = reinterpret_cast<const char*>(run.payload.data());
    const auto* found =
        std::search(p, p + run.payload.size(), std::begin(inc),
                    std::end(inc) - 1);
    if (found != p + run.payload.size() && found != p && clusz != 0u)
    {
        const size_t split = static_cast<size_t>(found - p);
        const std::span<const uint8_t> head(run.payload.data(), split);
        if (split >= k_min_dirty && looks_like_map(head))
        {
            unused_run map = run;
            map.last_cluster = static_cast<uint16_t>(
                run.first_cluster +
                static_cast<uint16_t>((split - 1u) / clusz));
            map.payload.assign(run.payload.begin(),
                               run.payload.begin() +
                                   static_cast<std::ptrdiff_t>(split));
            trim_trailing_nuls(map.payload);
            emit_run(std::move(map), out);

            unused_run src = run;
            const uint32_t delta = static_cast<uint32_t>(split / clusz);
            src.first_cluster = static_cast<uint16_t>(run.first_cluster + delta);
            src.payload.assign(run.payload.begin() +
                                   static_cast<std::ptrdiff_t>(split),
                               run.payload.end());
            emit_run(std::move(src), out);
            return;
        }
    }
    emit_run(std::move(run), out);
}

} /* namespace */

std::vector<unused_run> scan_unused_clusters(std::span<const uint8_t> volume,
                                             const bpb_info& bpb, fat_kind kind)
{
    std::vector<unused_run> out;
    if (!bpb.looks_valid || (kind != fat_kind::fat12 && kind != fat_kind::fat16) ||
        bpb.bytes_per_sector == 0u || bpb.sectors_per_cluster == 0u)
    {
        return out;
    }

    const uint32_t clusz = cluster_bytes(bpb);
    if (clusz == 0u)
    {
        return out;
    }
    const uint32_t nclus = data_cluster_count(bpb);
    if (nclus == 0u)
    {
        return out;
    }
    const uint32_t max_cluster = 1u + nclus;
    const size_t fat0_off =
        static_cast<size_t>(bpb.reserved_sectors) * bpb.bytes_per_sector;
    const size_t fat_bytes =
        static_cast<size_t>(bpb.sectors_per_fat_16) * bpb.bytes_per_sector;
    if (fat_bytes == 0u || fat0_off + fat_bytes > volume.size())
    {
        return out;
    }
    const std::span<const uint8_t> fat0 = volume.subspan(fat0_off, fat_bytes);

    unused_run cur{};
    bool open = false;

    for (uint32_t c = 2; c <= max_cluster; ++c)
    {
        uint16_t v = 0;
        if (!fat_get(fat0, kind, c, v))
        {
            break;
        }
        unused_kind k = unused_kind::fat_free;
        if (is_free_entry(kind, v))
        {
            k = unused_kind::fat_free;
        }
        else if (is_bad_entry(kind, v))
        {
            k = unused_kind::fat_bad;
        }
        else
        {
            if (open)
            {
                finish_run(cur, clusz, out);
                open = false;
                cur = unused_run{};
            }
            continue;
        }

        const size_t off = cluster_offset(bpb, c);
        if (off == static_cast<size_t>(-1) || off + clusz > volume.size())
        {
            if (open)
            {
                finish_run(cur, clusz, out);
                open = false;
                cur = unused_run{};
            }
            continue;
        }
        const std::span<const uint8_t> chunk = volume.subspan(off, clusz);
        if (!cluster_has_leftover(chunk))
        {
            if (open)
            {
                finish_run(cur, clusz, out);
                open = false;
                cur = unused_run{};
            }
            continue;
        }

        if (open && cur.kind == k &&
            static_cast<uint32_t>(cur.last_cluster) + 1u == c)
        {
            cur.last_cluster = static_cast<uint16_t>(c);
            cur.payload.insert(cur.payload.end(), chunk.begin(), chunk.end());
            continue;
        }
        if (open)
        {
            finish_run(cur, clusz, out);
        }
        cur = unused_run{};
        cur.first_cluster = static_cast<uint16_t>(c);
        cur.last_cluster = static_cast<uint16_t>(c);
        cur.kind = k;
        cur.payload.assign(chunk.begin(), chunk.end());
        open = true;
    }
    if (open)
    {
        finish_run(cur, clusz, out);
    }
    return out;
}

} /* namespace dumpfloppy */
