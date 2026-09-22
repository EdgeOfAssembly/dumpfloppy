/**
 * @file fat.cpp
 * @brief FAT12/16 table access and chain walking.
 */
#include "dumpfloppy/fat.hpp"
#include "dumpfloppy/bpb.hpp"
#include "dumpfloppy/fat12_codec.h"
#include "dumpfloppy/util.hpp"

#include <cstdio>
#include <unordered_set>

namespace dumpfloppy
{

bool fat_get(std::span<const uint8_t> fat, fat_kind kind, uint32_t cluster,
             uint16_t& out)
{
    if (kind == fat_kind::fat12)
    {
        uint16_t v = 0;
        if (fat12_entry_get(fat.data(), fat.size(), cluster, &v) != 0)
        {
            return false;
        }
        out = v;
        return true;
    }
    if (kind == fat_kind::fat16)
    {
        const size_t off = static_cast<size_t>(cluster) * 2u;
        if (off + 2u > fat.size())
        {
            return false;
        }
        out = read_le16(fat, off);
        return true;
    }
    return false;
}

std::vector<uint16_t> walk_chain(std::span<const uint8_t> fat, fat_kind kind,
                                 uint16_t start, uint32_t max_cluster,
                                 std::string& notes)
{
    std::vector<uint16_t> chain;
    notes.clear();
    if (start == 0u)
    {
        return chain;
    }
    if (start == 1u)
    {
        notes = "starts at reserved cluster 1";
        return chain;
    }

    std::unordered_set<uint16_t> seen;
    uint16_t cur = start;
    constexpr int k_max_steps = 65536;
    for (int step = 0; step < k_max_steps; ++step)
    {
        if (cur < 2u || cur > max_cluster)
        {
            notes = "chain left valid cluster range";
            break;
        }
        if (!seen.insert(cur).second)
        {
            notes = "cycle in cluster chain";
            break;
        }
        chain.push_back(cur);
        uint16_t next = 0;
        if (!fat_get(fat, kind, cur, next))
        {
            notes = "FAT truncated while walking chain";
            break;
        }
        if (kind == fat_kind::fat12)
        {
            if (fat12_is_eof(next))
            {
                break;
            }
            if (fat12_is_bad(next))
            {
                notes = "chain hits a bad cluster";
                break;
            }
            if (fat12_is_free(next))
            {
                notes = "chain hits a free cluster (truncated FAT chain)";
                break;
            }
        }
        else
        {
            if (next >= 0xFFF8u)
            {
                break;
            }
            if (next == 0xFFF7u)
            {
                notes = "chain hits a bad cluster";
                break;
            }
            if (next == 0u)
            {
                notes = "chain hits a free cluster (truncated FAT chain)";
                break;
            }
        }
        cur = next;
    }
    return chain;
}

size_t cluster_offset(const bpb_info& bpb, uint32_t cluster)
{
    if (cluster < 2u || bpb.bytes_per_sector == 0u)
    {
        return static_cast<size_t>(-1);
    }
    const uint32_t first = first_data_sector(bpb);
    const uint32_t sector =
        first + (cluster - 2u) * static_cast<uint32_t>(bpb.sectors_per_cluster);
    return static_cast<size_t>(sector) * bpb.bytes_per_sector;
}

fat_summary summarise_fat(std::span<const uint8_t> image, const bpb_info& bpb,
                          fat_kind kind)
{
    fat_summary s{};
    s.kind = kind;
    if (!bpb.looks_valid || bpb.bytes_per_sector == 0u)
    {
        return s;
    }

    s.cluster_count = data_cluster_count(bpb);
    s.max_cluster = (s.cluster_count == 0u) ? 1u : (1u + s.cluster_count);
    s.fat_bytes = static_cast<uint32_t>(bpb.sectors_per_fat_16) * bpb.bytes_per_sector;

    const size_t fat0_off =
        static_cast<size_t>(bpb.reserved_sectors) * bpb.bytes_per_sector;
    if (fat0_off + s.fat_bytes > image.size())
    {
        s.media_note = "FAT0 truncated by image length";
        return s;
    }
    const std::span<const uint8_t> fat0{image.data() + fat0_off, s.fat_bytes};

    if (bpb.fat_count >= 2u)
    {
        const size_t fat1_off = fat0_off + s.fat_bytes;
        if (fat1_off + s.fat_bytes <= image.size())
        {
            uint32_t mismatches = 0;
            for (uint32_t i = 0; i < s.fat_bytes; ++i)
            {
                if (image[fat0_off + i] != image[fat1_off + i])
                {
                    ++mismatches;
                }
            }
            s.copy_mismatch_bytes = mismatches;
            s.copies_match = (mismatches == 0u);
        }
        else
        {
            s.copies_match = false;
            s.media_note = "FAT1 truncated by image length";
        }
    }

    uint16_t e0 = 0;
    uint16_t e1 = 0;
    if (fat_get(fat0, kind, 0, e0))
    {
        s.fat0_media = e0;
    }
    if (fat_get(fat0, kind, 1, e1))
    {
        s.fat0_eoc = e1;
    }

    const uint32_t last = s.max_cluster;
    for (uint32_t c = 2; c <= last; ++c)
    {
        uint16_t v = 0;
        if (!fat_get(fat0, kind, c, v))
        {
            break;
        }
        if (kind == fat_kind::fat12)
        {
            if (fat12_is_free(v))
            {
                ++s.free_clusters;
            }
            else if (fat12_is_bad(v))
            {
                ++s.bad_clusters;
                if (s.bad_list.size() < 32u)
                {
                    s.bad_list.push_back(static_cast<uint16_t>(c));
                }
            }
            else if (fat12_is_reserved(v))
            {
                ++s.reserved_clusters;
            }
            else if (fat12_is_eof(v))
            {
                ++s.eof_markers;
                ++s.allocated_clusters;
            }
            else
            {
                ++s.allocated_clusters;
            }
        }
        else
        {
            if (v == 0u)
            {
                ++s.free_clusters;
            }
            else if (v == 0xFFF7u)
            {
                ++s.bad_clusters;
                if (s.bad_list.size() < 32u)
                {
                    s.bad_list.push_back(static_cast<uint16_t>(c));
                }
            }
            else if (v >= 0xFFF8u)
            {
                ++s.eof_markers;
                ++s.allocated_clusters;
            }
            else
            {
                ++s.allocated_clusters;
            }
        }
    }

    const uint8_t media_low = static_cast<uint8_t>(s.fat0_media & 0xFFu);
    if (media_low != bpb.media_descriptor)
    {
        char buf[80] = {};
        std::snprintf(buf, sizeof(buf),
                      "FAT[0] media 0x%02X differs from BPB 0x%02X", media_low,
                      bpb.media_descriptor);
        if (!s.media_note.empty())
        {
            s.media_note += "; ";
        }
        s.media_note += buf;
    }
    return s;
}

} /* namespace dumpfloppy */
