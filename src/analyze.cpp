/**
 * @file analyze.cpp
 * @brief Glue: BPB + boot + FAT + directory + volume serial/label + secrets.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/bpb.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/fat12_codec.h"
#include "dumpfloppy/util.hpp"

#include <algorithm>
#include <unordered_set>

namespace dumpfloppy
{
namespace
{

bool name_is(const dir_entry& e, std::string_view want)
{
    const std::string a = ascii_lower(e.name_83);
    return a == want;
}

std::string label_from_root(const std::vector<dir_entry>& entries)
{
    for (const dir_entry& e : entries)
    {
        if ((e.attributes & k_attr_volume) != 0u &&
            (e.attributes & k_attr_directory) == 0u && !e.deleted)
        {
            /* Volume labels are 11-char 8.3 without a real extension split. */
            if (!e.lfn.empty())
            {
                return e.lfn;
            }
            return e.name_83;
        }
    }
    return {};
}

} /* namespace */

analysis analyse(floppy_image image)
{
    analysis a{};
    a.image = std::move(image);
    const std::span<const uint8_t> bytes = a.image.bytes;
    const std::span<const uint8_t> boot =
        bytes.subspan(0, std::min<size_t>(512u, bytes.size()));

    a.bpb = parse_bpb(boot);
    a.ebpb = parse_ebpb(boot, a.bpb);
    a.boot = classify_boot(boot, a.bpb);
    a.kind = fat_kind_from_bpb(a.bpb);

    if (a.bpb.looks_valid)
    {
        a.volume_bytes = static_cast<uint64_t>(a.bpb.total_sectors) *
                         a.bpb.bytes_per_sector;
        if (a.volume_bytes > bytes.size())
        {
            a.truncated = true;
            a.secrets.emplace_back("image is shorter than BPB total sectors");
        }
        else if (bytes.size() > a.volume_bytes)
        {
            a.trailing_bytes = bytes.size() - a.volume_bytes;
            a.secrets.emplace_back("trailing bytes after BPB volume (overdump or WinImage extra)");
        }
    }

    if (a.ebpb.has_serial)
    {
        a.volume.serial_ebpb = a.ebpb.volume_serial;
        a.volume.serial_text = format_volume_serial(a.ebpb.volume_serial);
    }
    if (a.ebpb.has_label)
    {
        a.volume.label_ebpb = a.ebpb.volume_label;
    }

    if (a.bpb.looks_valid &&
        (a.kind == fat_kind::fat12 || a.kind == fat_kind::fat16))
    {
        a.fat = summarise_fat(bytes, a.bpb, a.kind);
        const size_t fat0_off =
            static_cast<size_t>(a.bpb.reserved_sectors) * a.bpb.bytes_per_sector;
        if (fat0_off < bytes.size() && a.fat.fat_bytes > 0u)
        {
            const size_t n = std::min<size_t>(a.fat.fat_bytes, bytes.size() - fat0_off);
            const std::span<const uint8_t> fat0{bytes.data() + fat0_off, n};
            a.entries = list_directories(bytes, a.bpb, a.kind, fat0);

            std::unordered_set<uint16_t> used;
            bool has_io = false;
            bool has_msdos = false;
            for (const dir_entry& e : a.entries)
            {
                for (uint16_t c : e.cluster_chain)
                {
                    used.insert(c);
                }
                if (!e.deleted && (e.path == "\\IO.SYS" || name_is(e, "io.sys") ||
                                   name_is(e, "ibmbio.com")))
                {
                    has_io = true;
                }
                if (!e.deleted && (name_is(e, "msdos.sys") || name_is(e, "ibmdos.com")))
                {
                    has_msdos = true;
                }
            }
            refine_boot_with_root(a.boot, has_io, has_msdos);

            for (uint32_t c = 2; c <= a.fat.max_cluster; ++c)
            {
                uint16_t v = 0;
                if (!fat_get(fat0, a.kind, c, v))
                {
                    break;
                }
                const bool allocated = (a.kind == fat_kind::fat12)
                                           ? (!fat12_is_free(v) && !fat12_is_bad(v) &&
                                              !fat12_is_reserved(v))
                                           : (v != 0u && v != 0xFFF7u);
                if (allocated && used.find(static_cast<uint16_t>(c)) == used.end())
                {
                    /* EOC markers on a cluster mean that cluster is in a chain;
                       walk_chain already recorded it. Remaining allocated slots
                       not in any chain are orphans. */
                    if (used.count(static_cast<uint16_t>(c)) == 0u)
                    {
                        a.orphan_clusters.push_back(static_cast<uint16_t>(c));
                        if (a.orphan_clusters.size() >= 64u)
                        {
                            break;
                        }
                    }
                }
            }
        }
    }

    a.volume.label_root = label_from_root(a.entries);
    if (!a.volume.label_root.empty())
    {
        a.volume.label_best = a.volume.label_root;
    }
    else
    {
        a.volume.label_best = a.volume.label_ebpb;
    }

    if (!a.volume.label_ebpb.empty() && !a.volume.label_root.empty() &&
        ascii_lower(a.volume.label_ebpb) != ascii_lower(a.volume.label_root))
    {
        a.secrets.emplace_back("EBPB volume label differs from root-directory label");
    }
    if (!a.fat.copies_match && a.bpb.fat_count >= 2u)
    {
        a.secrets.emplace_back("FAT copies differ");
    }
    if (!a.orphan_clusters.empty())
    {
        a.secrets.emplace_back("orphan clusters allocated in FAT but not in any directory chain");
    }
    for (const dir_entry& e : a.entries)
    {
        if (e.after_terminator)
        {
            a.secrets.emplace_back(
                "directory slots after the 0x00 terminator (undeleted leftovers)");
            break;
        }
    }
    if (a.bpb.looks_valid && a.image.size_geometry.sectors_per_track != 0u &&
        a.bpb.sectors_per_track != 0u &&
        a.bpb.sectors_per_track != a.image.size_geometry.sectors_per_track)
    {
        a.secrets.emplace_back("BPB sectors/track differs from size-inferred geometry");
    }
    if (a.ebpb.present && !a.ebpb.confident)
    {
        a.secrets.emplace_back(
            "extended BPB signature 0x29 present but FS type is not FATxx");
    }

    return a;
}

} /* namespace dumpfloppy */
