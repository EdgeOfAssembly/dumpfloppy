/**
 * @file offset.hpp
 * @brief Map one byte offset in a FAT12/FAT16 volume to sector and owner.
 *
 * Header-only so the lookup links from @c main.cpp without a new Makefile
 * object. Do not call this for other filesystems; the caller prints the
 * "FAT12/FAT16" diagnostic and exits 1.
 */
#ifndef DUMPFLOPPY_OFFSET_HPP
#define DUMPFLOPPY_OFFSET_HPP

#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/bpb.hpp"
#include "dumpfloppy/fat.hpp"

#include <cstdint>
#include <sstream>
#include <string>

namespace dumpfloppy
{
namespace
{

/**
 * @brief First directory entry whose cluster chain contains @p cluster.
 *
 * Live entries outrank deleted ones. A directory (size is often 0) counts
 * as owning its whole cluster. A file owns bytes below @a dir_entry::size;
 * bytes past that size in its cluster are slack, not a second file.
 */
struct fat_offset_owner
{
    const dir_entry* entry = nullptr;
    bool in_payload = false;
};

inline void consider_owner(fat_offset_owner& slot, const dir_entry& entry, bool in_payload)
{
    if (slot.entry == nullptr || (in_payload && !slot.in_payload))
    {
        slot.entry = &entry;
        slot.in_payload = in_payload;
    }
}

inline fat_offset_owner find_cluster_owner(const analysis& a, uint16_t cluster,
                                           uint64_t pos_in_cluster, uint64_t cluster_bytes)
{
    fat_offset_owner live{};
    fat_offset_owner dead{};
    for (const dir_entry& entry : a.entries)
    {
        if (entry.name_83 == "." || entry.name_83 == ".." || entry.is_lfn_orphan)
        {
            continue;
        }
        if ((entry.attributes & k_attr_volume) != 0u &&
            (entry.attributes & k_attr_directory) == 0u)
        {
            continue;
        }
        uint32_t index = 0;
        bool found = false;
        for (const uint16_t link : entry.cluster_chain)
        {
            if (link == cluster)
            {
                found = true;
                break;
            }
            ++index;
        }
        if (!found)
        {
            continue;
        }
        const bool is_dir = (entry.attributes & k_attr_directory) != 0u;
        const uint64_t logical =
            (static_cast<uint64_t>(index) * cluster_bytes) + pos_in_cluster;
        const bool in_payload = is_dir || (logical < static_cast<uint64_t>(entry.size));
        consider_owner(entry.deleted ? dead : live, entry, in_payload);
    }
    return (live.entry != nullptr) ? live : dead;
}

} /* namespace */

/**
 * @brief Describe @p offset inside a FAT12 or FAT16 image.
 *
 * The line is `OFFSET sector SECTOR reserved/FAT/root`, or
 * `OFFSET sector SECTOR cluster CLUSTER NAME|free|slack`, or
 * `OFFSET sector SECTOR past-end`. @p offset is a byte index into the
 * logical volume (@ref volume_bytes): the image file itself for a raw
 * `.img` / `.ima`, or the assembled CHS buffer when flux was decoded.
 *
 * @param[in] a      Analysis with a parsed BPB, FAT summary, and directory.
 * @param[in] offset Byte offset from the start of that volume.
 * @return One line, without a trailing newline.
 *
 * @pre @a a.kind is @ref fat_kind::fat12 or @ref fat_kind::fat16.
 */
[[nodiscard]] inline std::string format_fat_offset(const analysis& a, uint64_t offset)
{
    std::ostringstream line;
    const uint64_t bps = a.bpb.bytes_per_sector;
    const uint64_t volume_size = static_cast<uint64_t>(volume_bytes(a).size());
    const uint64_t fs_bytes = static_cast<uint64_t>(a.bpb.total_sectors) * bps;
    const bool past = (bps == 0u) || (offset >= volume_size) ||
                      (fs_bytes != 0u && offset >= fs_bytes);
    if (past)
    {
        line << offset;
        if (bps != 0u)
        {
            line << " sector " << (offset / bps);
        }
        line << " past-end";
        return line.str();
    }

    const uint64_t sector = offset / bps;
    const uint64_t first = first_data_sector(a.bpb);
    const uint64_t spc = a.bpb.sectors_per_cluster;
    if (sector < first || spc == 0u)
    {
        line << offset << " sector " << sector << " reserved/FAT/root";
        return line.str();
    }

    const uint64_t cluster = 2u + ((sector - first) / spc);
    if (cluster > static_cast<uint64_t>(a.fat.max_cluster) || cluster > 0xFFFFu)
    {
        line << offset << " sector " << sector << " past-end";
        return line.str();
    }

    const uint64_t cluster_bytes = bps * spc;
    const std::size_t base = cluster_offset(a.bpb, static_cast<uint32_t>(cluster));
    if (base == static_cast<std::size_t>(-1) || offset < static_cast<uint64_t>(base))
    {
        line << offset << " sector " << sector << " past-end";
        return line.str();
    }
    const uint64_t pos_in_cluster = offset - static_cast<uint64_t>(base);
    const fat_offset_owner owner = find_cluster_owner(
        a, static_cast<uint16_t>(cluster), pos_in_cluster, cluster_bytes);

    line << offset << " sector " << sector << " cluster " << cluster << ' ';
    if (owner.entry == nullptr)
    {
        line << "free";
    }
    else if (!owner.in_payload)
    {
        line << "slack";
    }
    else if (!owner.entry->path.empty())
    {
        line << owner.entry->path;
    }
    else
    {
        line << owner.entry->name_83;
    }
    return line.str();
}

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_OFFSET_HPP */
