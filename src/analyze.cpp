/**
 * @file analyze.cpp
 * @brief Glue: BPB + boot + FAT + directory + volume serial/label + secrets.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/amiga.hpp"
#include "dumpfloppy/bpb.hpp"
#include "dumpfloppy/catalog.hpp"
#include "dumpfloppy/cbm.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/fat.hpp"
#include "dumpfloppy/fat12_codec.h"
#include "dumpfloppy/format_registry.hpp"
#include "dumpfloppy/g64.hpp"
#include "dumpfloppy/ibm_mfm.hpp"
#include "dumpfloppy/trd.hpp"
#include "dumpfloppy/util.hpp"
#include "dumpfloppy/volume.hpp"

#include <algorithm>
#include <span>
#include <unordered_set>
#include <vector>

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

    a.catalog = catalog_lookup(a.image.xxh64);
    a.cbm = parse_g64(bytes);
    if (!a.cbm.present)
    {
        a.cbm = parse_cbm_image(bytes);
    }
    if (a.cbm.present)
    {
        /* CBMFS D64/D71/D81/G64 is not an IBM BPB/FAT volume and not HxC flux. */
        a.image.size_geometry.bytes_per_sector = k_d64_sector_bytes;
        switch (a.cbm.media)
        {
        case cbm_media::g64:
            a.image.size_geometry.cylinders = 0;
            a.image.size_geometry.heads = 0;
            a.image.size_geometry.sectors_per_track = 0;
            a.image.size_geometry.expected_bytes = k_d64_35_bytes;
            a.image.size_geometry.media_name = "Commodore 1541 G64 (GCR-1541)";
            break;
        case cbm_media::d71:
            a.image.size_geometry.cylinders = 0;
            a.image.size_geometry.heads = 0;
            a.image.size_geometry.sectors_per_track = 0;
            a.image.size_geometry.expected_bytes = k_d71_bytes;
            a.image.size_geometry.media_name =
                (bytes.size() == k_d71_error_bytes)
                    ? "Commodore 1571 70-track D71 + error map"
                    : "Commodore 1571 70-track D71";
            break;
        case cbm_media::d81:
            a.image.size_geometry.cylinders = 80;
            a.image.size_geometry.heads = 1;
            a.image.size_geometry.sectors_per_track = 40;
            a.image.size_geometry.expected_bytes = k_d81_bytes;
            a.image.size_geometry.media_name =
                (bytes.size() == k_d81_error_bytes)
                    ? "Commodore 1581 80-track D81 + error map"
                    : "Commodore 1581 80-track D81";
            break;
        case cbm_media::d64:
        default:
            a.image.size_geometry.cylinders = 0;
            a.image.size_geometry.heads = 0;
            a.image.size_geometry.sectors_per_track = 0;
            a.image.size_geometry.expected_bytes = k_d64_35_bytes;
            a.image.size_geometry.media_name =
                (bytes.size() == k_d64_35_error_bytes)
                    ? "Commodore 1541 35-track D64 + error map"
                    : "Commodore 1541 35-track D64";
            break;
        }
        return a;
    }

    a.amiga = parse_adf(bytes);
    if (a.amiga.present)
    {
        /* Amiga OFS/FFS ADF is not an IBM BPB/FAT volume and not HxC flux. */
        a.image.size_geometry.cylinders = 80;
        a.image.size_geometry.heads = 2;
        a.image.size_geometry.bytes_per_sector = k_adf_sector_bytes;
        if (is_adf_hd_size(bytes.size()))
        {
            a.image.size_geometry.sectors_per_track = 22;
            a.image.size_geometry.expected_bytes = k_adf_hd_bytes;
            a.image.size_geometry.media_name = "Amiga HD ADF (80×2×22×512)";
        }
        else
        {
            a.image.size_geometry.sectors_per_track = 11;
            a.image.size_geometry.expected_bytes = k_adf_dd_bytes;
            a.image.size_geometry.media_name = "Amiga DD ADF (80×2×11×512)";
        }
        return a;
    }

    a.trd = parse_trd(bytes);
    if (a.trd.present)
    {
        a.image.size_geometry.cylinders = a.trd.cylinders;
        a.image.size_geometry.heads = a.trd.sides;
        a.image.size_geometry.sectors_per_track = k_trd_spt;
        a.image.size_geometry.bytes_per_sector = k_trd_sector_bytes;
        a.image.size_geometry.expected_bytes = bytes.size();
        a.image.size_geometry.media_name =
            std::string("ZX Spectrum TR-DOS TRD ") + a.trd.disk_type_name;
        return a;
    }

    a.flux = decode_hxc_mfm(bytes);
    if (!a.flux.present)
    {
        a.flux = inspect_86f(bytes);
    }

    std::span<const uint8_t> boot_span = boot;
    if (a.flux.present && a.flux.boot.size() >= 32u)
    {
        boot_span = std::span<const uint8_t>(a.flux.boot);
    }

    a.bpb = parse_bpb(boot_span);
    a.ebpb = parse_ebpb(boot_span, a.bpb);
    a.boot = classify_boot(boot_span, a.bpb);
    if (a.flux.present && !a.bpb.looks_valid && !a.flux.boot.empty() &&
        !a.boot.is_booter)
    {
        a.boot.is_booter = true;
        a.boot.kind = boot_class::custom_booter;
        a.boot.kind_text = "custom / game booter (decoded IBM MFM track 0)";
    }
    a.kind = fat_kind_from_bpb(a.bpb);

    /* Raw HxC/86F bytes are a bitstream, not a FAT volume. Walking the
       directory against image.bytes invents garbage 8.3 names. */
    const bool flux_without_chs =
        a.flux.present && a.flux.assembled_chs.empty();
    if (flux_without_chs)
    {
        a.secrets.emplace_back(
            "CHS assembly failed; flux bitstream is not a FAT volume");
    }

    const sector_store store = make_sector_store(a);
    const std::span<const uint8_t> volume = store.bytes;

    if (a.bpb.looks_valid && !flux_without_chs)
    {
        a.volume_bytes = static_cast<uint64_t>(a.bpb.total_sectors) *
                         a.bpb.bytes_per_sector;
        if (a.volume_bytes > volume.size())
        {
            a.truncated = true;
            a.secrets.emplace_back("image is shorter than BPB total sectors");
        }
        else if (!a.flux.present && bytes.size() > a.volume_bytes)
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

    if (a.bpb.looks_valid && !flux_without_chs &&
        (a.kind == fat_kind::fat12 || a.kind == fat_kind::fat16))
    {
        a.fat = summarise_fat(volume, a.bpb, a.kind);
        const size_t fat0_off =
            static_cast<size_t>(a.bpb.reserved_sectors) * a.bpb.bytes_per_sector;
        if (fat0_off < volume.size() && a.fat.fat_bytes > 0u)
        {
            const size_t n = std::min<size_t>(a.fat.fat_bytes, volume.size() - fat0_off);
            const std::span<const uint8_t> fat0{volume.data() + fat0_off, n};
            a.entries = list_directories(volume, a.bpb, a.kind, fat0);
            for (dir_entry& e : a.entries)
            {
                if (e.name_83 == "." || e.name_83 == ".." ||
                    (e.attributes & k_attr_directory) != 0u)
                {
                    e.type = "DIR";
                    continue;
                }
                if ((e.attributes & k_attr_volume) != 0u)
                {
                    e.type = "VOL";
                    e.xxh64.clear();
                    continue;
                }
                if (e.size == 0u)
                {
                    e.xxh64.clear();
                    e.type = identify_type({}, format_kind::file, e.name_83);
                    continue;
                }
                const std::vector<uint8_t> payload =
                    read_file_contents(volume, a.bpb, e);
                e.xxh64 = xxh64_hex(payload);
                e.type = identify_type(payload, format_kind::file, e.name_83);
            }

            std::unordered_set<uint16_t> used;
            bool has_io = false;
            bool has_msdos = false;
            for (const dir_entry& e : a.entries)
            {
                for (uint16_t c : e.cluster_chain)
                {
                    used.insert(c);
                }
                if (!e.deleted && (name_is(e, "io.sys") || name_is(e, "ibmbio.com")))
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

std::span<const uint8_t> volume_bytes(const analysis& a)
{
    return make_ibm_store(a.flux.assembled_chs, a.image.bytes,
                          a.bpb.bytes_per_sector)
        .bytes;
}

std::vector<uint8_t>& volume_bytes_mut(analysis& a)
{
    return ibm_volume_mut(a.flux.assembled_chs, a.image.bytes);
}

sector_store make_sector_store(const analysis& a)
{
    return make_ibm_store(a.flux.assembled_chs, a.image.bytes,
                          a.bpb.bytes_per_sector);
}

} /* namespace dumpfloppy */
