/**
 * @file forensics.cpp
 * @brief Slack tails, leaked directory slots, and signature carving.
 */
#include "dumpfloppy/forensics.hpp"

#include "dumpfloppy/bpb.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/fat.hpp"
#include "dumpfloppy/fat12_codec.h"
#include "dumpfloppy/fat_slack.h"
#include "dumpfloppy/util.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <ostream>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace dumpfloppy
{
namespace
{

constexpr std::size_t k_max_hits = 64;
constexpr unsigned k_attr_rhsvda = 0x37u;

struct byte_span
{
    std::size_t off = 0;
    std::size_t len = 0;
    /** @brief Origin for 32-byte directory slots. Leaked scan only. */
    std::size_t align_base = 0;
};

struct carve_hit
{
    std::size_t off = 0;
    const char* kind = "";
    std::size_t length = 0;
    std::string text{};
};

[[nodiscard]] bool ranges_overlap(std::size_t a, std::size_t an, std::size_t b,
                                  std::size_t bn)
{
    if (an == 0u || bn == 0u)
    {
        return false;
    }
    const std::size_t ae = a + an;
    const std::size_t be = b + bn;
    return a < be && b < ae;
}

[[nodiscard]] bool overlaps_any(std::size_t off, std::size_t n,
                                const std::vector<byte_span>& dirs)
{
    for (const byte_span& d : dirs)
    {
        if (ranges_overlap(off, n, d.off, d.len))
        {
            return true;
        }
    }
    return false;
}

[[nodiscard]] uint32_t cluster_bytes_of(const bpb_info& bpb)
{
    return static_cast<uint32_t>(bpb.bytes_per_sector) *
           static_cast<uint32_t>(bpb.sectors_per_cluster);
}

void append_hex(std::ostream& out, std::span<const uint8_t> bytes)
{
    static constexpr char k_hex[] = "0123456789abcdef";
    for (const uint8_t b : bytes)
    {
        out << k_hex[b >> 4] << k_hex[b & 0x0Fu];
    }
}

[[nodiscard]] std::string escape_slot_name(const uint8_t raw[11])
{
    auto trim = [](const uint8_t* p, int n)
    {
        while (n > 0 && p[n - 1] == static_cast<uint8_t>(' '))
        {
            --n;
        }
        return n;
    };
    const int nb = trim(raw, 8);
    const int ne = trim(raw + 8, 3);
    std::string shown;
    shown.reserve(16);
    for (int i = 0; i < nb; ++i)
    {
        shown.push_back(static_cast<char>(raw[i]));
    }
    if (ne > 0)
    {
        shown.push_back('.');
        for (int i = 0; i < ne; ++i)
        {
            shown.push_back(static_cast<char>(raw[8 + i]));
        }
    }
    std::string out;
    out.push_back('\'');
    for (unsigned char c : shown)
    {
        if (c >= 0x20u && c <= 0x7Eu && c != '\\' && c != '\'')
        {
            out.push_back(static_cast<char>(c));
        }
        else
        {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "\\x%02x", static_cast<unsigned>(c));
            out += buf;
        }
    }
    out.push_back('\'');
    return out;
}

[[nodiscard]] bool name_has_non_space(const uint8_t* name8)
{
    for (int i = 0; i < 8; ++i)
    {
        if (name8[i] != static_cast<uint8_t>(' '))
        {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool slot_is_leaked(std::span<const uint8_t> slot, uint32_t max_cluster,
                                  uint64_t volume_size)
{
    if (slot.size() < 32u)
    {
        return false;
    }
    if (slot[0] == 0x00u)
    {
        return false;
    }
    if (!name_has_non_space(slot.data()))
    {
        return false;
    }
    const uint8_t attr = slot[11];
    if (attr != k_attr_lfn && (attr & static_cast<uint8_t>(~k_attr_rhsvda)) != 0u)
    {
        return false;
    }
    const uint16_t cluster = read_le16(slot, 26);
    if (cluster != 0u && (cluster < 2u || static_cast<uint32_t>(cluster) > max_cluster))
    {
        return false;
    }
    const uint32_t size = read_le32(slot, 28);
    if (static_cast<uint64_t>(size) > volume_size)
    {
        return false;
    }
    return true;
}

void collect_directory_ranges(const analysis& a, std::span<const uint8_t> vol,
                              std::vector<byte_span>& dirs)
{
    const bpb_info& bpb = a.bpb;
    const std::size_t root_off =
        (static_cast<std::size_t>(bpb.reserved_sectors) +
         static_cast<std::size_t>(bpb.fat_count) * bpb.sectors_per_fat_16) *
        bpb.bytes_per_sector;
    const std::size_t root_bytes = static_cast<std::size_t>(bpb.root_entry_count) * 32u;
    if (root_off < vol.size() && root_bytes > 0u)
    {
        const std::size_t n = std::min(root_bytes, vol.size() - root_off);
        dirs.push_back(byte_span{root_off, n});
    }
    const uint32_t cluster_bytes = cluster_bytes_of(bpb);
    for (const dir_entry& e : a.entries)
    {
        if ((e.attributes & k_attr_directory) == 0u || e.deleted)
        {
            continue;
        }
        if (e.name_83 == "." || e.name_83 == "..")
        {
            continue;
        }
        for (const uint16_t cl : e.cluster_chain)
        {
            const std::size_t off = cluster_offset(bpb, cl);
            if (off == static_cast<std::size_t>(-1) || off >= vol.size() ||
                cluster_bytes == 0u)
            {
                continue;
            }
            const std::size_t n =
                std::min(static_cast<std::size_t>(cluster_bytes), vol.size() - off);
            dirs.push_back(byte_span{off, n});
        }
    }
}

void collect_slack(const analysis& a, std::span<const uint8_t> vol,
                   std::vector<byte_span>& regions, std::ostream& out, bool print)
{
    const uint32_t cluster_bytes = cluster_bytes_of(a.bpb);
    if (print)
    {
        out << "=== Slack ===\n";
    }
    for (const dir_entry& e : a.entries)
    {
        if (e.deleted || !is_payload_file(e))
        {
            continue;
        }
        const uint32_t slack = fat_slack_bytes(e.size, cluster_bytes);
        if (slack == 0u || cluster_bytes == 0u)
        {
            continue;
        }
        const uint32_t needed = (e.size / cluster_bytes) + 1u;
        if (e.cluster_chain.size() < needed)
        {
            continue;
        }
        const uint16_t last = e.cluster_chain[needed - 1u];
        const std::size_t cl_off = cluster_offset(a.bpb, last);
        if (cl_off == static_cast<std::size_t>(-1) || cl_off >= vol.size())
        {
            continue;
        }
        const uint32_t used = e.size % cluster_bytes;
        const std::size_t slack_off = cl_off + static_cast<std::size_t>(used);
        if (slack_off >= vol.size())
        {
            continue;
        }
        const std::size_t room = vol.size() - slack_off;
        const std::size_t cluster_room =
            (cl_off + static_cast<std::size_t>(cluster_bytes) > slack_off)
                ? (cl_off + static_cast<std::size_t>(cluster_bytes) - slack_off)
                : 0u;
        const std::size_t n =
            std::min(room, std::min(cluster_room, static_cast<std::size_t>(slack)));
        if (n == 0u)
        {
            continue;
        }
        regions.push_back(byte_span{slack_off, n, cl_off});
        if (print)
        {
            const std::string path = e.path.empty() ? e.name_83 : e.path;
            out << path << ' ' << slack_off << ' ' << n << ' ';
            append_hex(out, std::span<const uint8_t>(vol.data() + slack_off, n));
            out << '\n';
        }
    }
}

void collect_free_clusters(const analysis& a, std::span<const uint8_t> vol,
                           std::span<const uint8_t> fat, std::vector<byte_span>& regions)
{
    const uint32_t cluster_bytes = cluster_bytes_of(a.bpb);
    if (fat.empty() || cluster_bytes == 0u || a.fat.max_cluster < 2u)
    {
        return;
    }
    for (uint32_t c = 2; c <= a.fat.max_cluster; ++c)
    {
        uint16_t v = 0;
        if (!fat_get(fat, a.kind, c, v))
        {
            break;
        }
        const bool free =
            (a.kind == fat_kind::fat12) ? (fat12_is_free(v) != 0) : (v == 0u);
        if (!free)
        {
            continue;
        }
        const std::size_t off = cluster_offset(a.bpb, c);
        if (off == static_cast<std::size_t>(-1) || off >= vol.size())
        {
            continue;
        }
        const std::size_t n =
            std::min(static_cast<std::size_t>(cluster_bytes), vol.size() - off);
        regions.push_back(byte_span{off, n, off});
    }
}

void add_past_end(const analysis& a, std::span<const uint8_t> vol,
                  std::vector<byte_span>& regions)
{
    if (a.volume_bytes >= vol.size())
    {
        return;
    }
    const std::size_t off = static_cast<std::size_t>(a.volume_bytes);
    /* Past-end slots stay on the volume's 32-byte grid (base 0). */
    regions.push_back(byte_span{off, vol.size() - off, 0u});
}

struct leaked_hit
{
    std::size_t off = 0;
    std::string name{};
    uint8_t attr = 0;
    uint16_t cluster = 0;
    uint32_t size = 0;
};

/**
 * @brief First offset at or after @p region_off with `(off - align_base) % 32 == 0`.
 */
[[nodiscard]] std::size_t aligned_slot_off(std::size_t region_off, std::size_t align_base)
{
    std::size_t mis = 0;
    if (region_off >= align_base)
    {
        mis = (region_off - align_base) % 32u;
    }
    else
    {
        const std::size_t back = (align_base - region_off) % 32u;
        mis = (back == 0u) ? 0u : (32u - back);
    }
    if (mis == 0u)
    {
        return region_off;
    }
    return region_off + (32u - mis);
}

void scan_leaked(std::span<const uint8_t> vol, const std::vector<byte_span>& regions,
                 const std::vector<byte_span>& dirs, uint32_t max_cluster,
                 uint64_t volume_size, std::vector<leaked_hit>& hits, bool& truncated)
{
    for (const byte_span& region : regions)
    {
        if (truncated)
        {
            return;
        }
        const std::size_t end = region.off + region.len;
        std::size_t off = aligned_slot_off(region.off, region.align_base);
        while (off >= region.off && end >= off && end - off >= 32u &&
               vol.size() >= off && vol.size() - off >= 32u)
        {
            if (overlaps_any(off, 32u, dirs))
            {
                off += 32u;
                continue;
            }
            const std::span<const uint8_t> slot(vol.data() + off, 32u);
            if (!slot_is_leaked(slot, max_cluster, volume_size))
            {
                off += 32u;
                continue;
            }
            if (hits.size() >= k_max_hits)
            {
                truncated = true;
                return;
            }
            leaked_hit hit;
            hit.off = off;
            hit.name = escape_slot_name(slot.data());
            hit.attr = slot[11];
            hit.cluster = read_le16(slot, 26);
            hit.size = read_le32(slot, 28);
            hits.push_back(std::move(hit));
            off += 32u;
        }
    }
}

[[nodiscard]] bool bytes_match(std::span<const uint8_t> vol, std::size_t off,
                               const char* lit, std::size_t n,
                               const std::vector<byte_span>& dirs)
{
    if (off + n > vol.size() || overlaps_any(off, n, dirs))
    {
        return false;
    }
    for (std::size_t i = 0; i < n; ++i)
    {
        if (vol[off + i] != static_cast<uint8_t>(lit[i]))
        {
            return false;
        }
    }
    return true;
}

void add_carve(std::vector<carve_hit>& hits, bool& truncated, std::size_t off,
               const char* kind, std::size_t length, std::string text)
{
    if (hits.size() >= k_max_hits)
    {
        truncated = true;
        return;
    }
    carve_hit hit;
    hit.off = off;
    hit.kind = kind;
    hit.length = length;
    if (text.size() > 48u)
    {
        text.resize(48);
    }
    hit.text = std::move(text);
    hits.push_back(std::move(hit));
}

void scan_carve_region(std::span<const uint8_t> vol, const byte_span& region,
                       const std::vector<byte_span>& dirs, std::vector<carve_hit>& hits,
                       bool& truncated)
{
    const std::size_t end = std::min(region.off + region.len, vol.size());
    if (region.off >= end)
    {
        return;
    }
    std::size_t ascii_at = static_cast<std::size_t>(-1);
    auto flush_ascii = [&](std::size_t stop)
    {
        if (ascii_at == static_cast<std::size_t>(-1) || stop < ascii_at)
        {
            ascii_at = static_cast<std::size_t>(-1);
            return;
        }
        const std::size_t len = stop - ascii_at;
        if (len >= 16u && !truncated)
        {
            std::string text;
            text.reserve(std::min<std::size_t>(len, 48u));
            const std::size_t show = std::min<std::size_t>(len, 48u);
            for (std::size_t i = 0; i < show; ++i)
            {
                text.push_back(static_cast<char>(vol[ascii_at + i]));
            }
            add_carve(hits, truncated, ascii_at, "ASCII", len, std::move(text));
        }
        ascii_at = static_cast<std::size_t>(-1);
    };

    for (std::size_t off = region.off; off < end; ++off)
    {
        if (truncated)
        {
            return;
        }
        if (overlaps_any(off, 1u, dirs))
        {
            flush_ascii(off);
            continue;
        }
        if (bytes_match(vol, off, "GIF87a", 6u, dirs))
        {
            add_carve(hits, truncated, off, "GIF87a", 6u, "GIF87a");
        }
        else if (bytes_match(vol, off, "GIF89a", 6u, dirs))
        {
            add_carve(hits, truncated, off, "GIF89a", 6u, "GIF89a");
        }
        else if (bytes_match(vol, off, "MZ", 2u, dirs))
        {
            add_carve(hits, truncated, off, "MZ", 2u, "MZ");
        }
        else if (bytes_match(vol, off, "ZM", 2u, dirs))
        {
            add_carve(hits, truncated, off, "ZM", 2u, "ZM");
        }
        if (truncated)
        {
            return;
        }
        const uint8_t b = vol[off];
        if (b >= 0x20u && b <= 0x7Eu)
        {
            if (ascii_at == static_cast<std::size_t>(-1))
            {
                ascii_at = off;
            }
        }
        else
        {
            flush_ascii(off);
        }
    }
    flush_ascii(end);
}

} /* namespace */

int write_forensics(const analysis& a, const forensics_request& req, std::ostream& out,
                    std::ostream& err)
{
    if (!req.slack && !req.leaked && !req.carve)
    {
        return 0;
    }
    if (a.kind != fat_kind::fat12 && a.kind != fat_kind::fat16)
    {
        if (req.slack)
        {
            err << "dumpfloppy: slack is only implemented for FAT12/FAT16\n";
        }
        if (req.leaked)
        {
            err << "dumpfloppy: leaked is only implemented for FAT12/FAT16\n";
        }
        if (req.carve)
        {
            err << "dumpfloppy: carve is only implemented for FAT12/FAT16\n";
        }
        return 1;
    }

    const std::span<const uint8_t> vol = volume_bytes(a);
    std::vector<byte_span> dirs;
    collect_directory_ranges(a, vol, dirs);

    std::vector<byte_span> slack_regions;
    collect_slack(a, vol, slack_regions, out, req.slack);

    std::span<const uint8_t> fat{};
    const std::size_t fat0_off =
        static_cast<std::size_t>(a.bpb.reserved_sectors) * a.bpb.bytes_per_sector;
    if (fat0_off < vol.size() && a.fat.fat_bytes > 0u)
    {
        const std::size_t n = std::min<std::size_t>(a.fat.fat_bytes, vol.size() - fat0_off);
        fat = std::span<const uint8_t>(vol.data() + fat0_off, n);
    }

    std::vector<byte_span> scan = slack_regions;
    collect_free_clusters(a, vol, fat, scan);
    add_past_end(a, vol, scan);

    if (req.leaked)
    {
        std::vector<leaked_hit> hits;
        bool truncated = false;
        const uint64_t volume_size =
            (a.volume_bytes != 0u)
                ? a.volume_bytes
                : (static_cast<uint64_t>(a.bpb.total_sectors) * a.bpb.bytes_per_sector);
        scan_leaked(vol, scan, dirs, a.fat.max_cluster, volume_size, hits, truncated);
        out << "=== Leaked directory entries ===\n";
        for (const leaked_hit& hit : hits)
        {
            char attr[8];
            std::snprintf(attr, sizeof(attr), "%02x", static_cast<unsigned>(hit.attr));
            out << hit.off << ' ' << hit.name << ' ' << attr << ' '
                << static_cast<unsigned>(hit.cluster) << ' ' << hit.size << '\n';
        }
        if (truncated)
        {
            err << "Warning: leaked directory scan stopped at cap\n";
        }
    }

    if (req.carve)
    {
        std::vector<carve_hit> hits;
        bool truncated = false;
        for (const byte_span& region : scan)
        {
            if (truncated)
            {
                break;
            }
            scan_carve_region(vol, region, dirs, hits, truncated);
        }
        out << "=== Carve ===\n";
        for (const carve_hit& hit : hits)
        {
            out << hit.off << ' ' << hit.kind << ' ' << hit.length << ' ' << hit.text
                << '\n';
        }
        if (truncated)
        {
            err << "Warning: carve scan stopped at cap\n";
        }
    }
    return 0;
}

} /* namespace dumpfloppy */
