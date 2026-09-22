/**
 * @file update.cpp
 * @brief FAT12/16 in-place replace, shrink, grow, and live-file relocate.
 */
#include "dumpfloppy/update.hpp"
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/bpb.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/fat.hpp"
#include "dumpfloppy/fat12_codec.h"
#include "dumpfloppy/util.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <ostream>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

namespace dumpfloppy
{
namespace
{

void poke_le16(uint8_t* p, uint16_t v)
{
    p[0] = static_cast<uint8_t>(v & 0xFFu);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
}

void poke_le32(uint8_t* p, uint32_t v)
{
    p[0] = static_cast<uint8_t>(v & 0xFFu);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
    p[2] = static_cast<uint8_t>((v >> 16) & 0xFFu);
    p[3] = static_cast<uint8_t>((v >> 24) & 0xFFu);
}

[[nodiscard]] uint16_t eof_marker(fat_kind kind)
{
    return (kind == fat_kind::fat16) ? static_cast<uint16_t>(0xFFFFu)
                                     : static_cast<uint16_t>(0x0FFFu);
}

[[nodiscard]] bool cluster_is_free(std::span<const uint8_t> fat, fat_kind kind,
                                   uint32_t cluster)
{
    uint16_t v = 0;
    if (!fat_get(fat, kind, cluster, v))
    {
        return false;
    }
    if (kind == fat_kind::fat12)
    {
        return fat12_is_free(v) != 0;
    }
    return v == 0u;
}

/**
 * @brief True when FAT marks @p cluster in-use (next/EOC), not free/bad/reserved.
 *
 * @param[in] fat     FAT0 bytes.
 * @param[in] kind    FAT12 or FAT16.
 * @param[in] cluster Cluster index.
 *
 * @retval true  Allocated to a chain (including EOC).
 * @retval false Free, bad, reserved, or unreadable.
 */
[[nodiscard]] bool cluster_is_allocated(std::span<const uint8_t> fat, fat_kind kind,
                                        uint32_t cluster)
{
    uint16_t v = 0;
    if (!fat_get(fat, kind, cluster, v))
    {
        return false;
    }
    if (kind == fat_kind::fat12)
    {
        return fat12_is_free(v) == 0 && fat12_is_bad(v) == 0 &&
               fat12_is_reserved(v) == 0;
    }
    if (v == 0u || v == 0xFFF7u)
    {
        return false;
    }
    if (v >= 0xFFF0u && v <= 0xFFF6u)
    {
        return false;
    }
    return true;
}

void free_chain(std::span<uint8_t> fat, fat_kind kind, const std::vector<uint16_t>& chain)
{
    for (uint16_t c : chain)
    {
        (void)fat_set(fat, kind, c, 0);
    }
}

bool link_chain(std::span<uint8_t> fat, fat_kind kind, const std::vector<uint16_t>& chain)
{
    if (chain.empty())
    {
        return true;
    }
    const uint16_t eoc = eof_marker(kind);
    for (size_t i = 0; i + 1u < chain.size(); ++i)
    {
        if (!fat_set(fat, kind, chain[i], chain[i + 1u]))
        {
            return false;
        }
    }
    return fat_set(fat, kind, chain.back(), eoc);
}

bool write_dir_slot(std::vector<uint8_t>& volume, size_t slot_off, uint16_t first,
                    uint32_t size)
{
    if (slot_off == 0u || slot_off + 32u > volume.size())
    {
        return false;
    }
    poke_le16(volume.data() + slot_off + 26u, first);
    poke_le32(volume.data() + slot_off + 28u, size);
    return true;
}

void sync_fat_copies(std::vector<uint8_t>& volume, const bpb_info& bpb)
{
    if (bpb.fat_count < 2u || bpb.bytes_per_sector == 0u)
    {
        return;
    }
    const size_t fat_bytes =
        static_cast<size_t>(bpb.sectors_per_fat_16) * bpb.bytes_per_sector;
    const size_t fat0 =
        static_cast<size_t>(bpb.reserved_sectors) * bpb.bytes_per_sector;
    const size_t fat1 = fat0 + fat_bytes;
    if (fat0 + fat_bytes > volume.size() || fat1 + fat_bytes > volume.size())
    {
        return;
    }
    std::memcpy(volume.data() + fat1, volume.data() + fat0, fat_bytes);
}

bool write_chain_payload(std::vector<uint8_t>& volume, const bpb_info& bpb,
                         const std::vector<uint16_t>& chain,
                         std::span<const uint8_t> payload)
{
    const uint32_t cluster_bytes =
        static_cast<uint32_t>(bpb.bytes_per_sector) * bpb.sectors_per_cluster;
    if (cluster_bytes == 0u)
    {
        return false;
    }
    size_t done = 0;
    for (uint16_t cl : chain)
    {
        const size_t off = cluster_offset(bpb, cl);
        if (off >= volume.size())
        {
            return false;
        }
        const size_t room = std::min<size_t>(cluster_bytes, volume.size() - off);
        std::memset(volume.data() + off, 0, room);
        if (done < payload.size())
        {
            const size_t n = std::min(room, payload.size() - done);
            std::memcpy(volume.data() + off, payload.data() + done, n);
            done += n;
        }
    }
    return done >= payload.size() || payload.empty();
}

std::span<uint8_t> fat0_span(std::vector<uint8_t>& volume, const bpb_info& bpb)
{
    const size_t fat_bytes =
        static_cast<size_t>(bpb.sectors_per_fat_16) * bpb.bytes_per_sector;
    const size_t fat0 =
        static_cast<size_t>(bpb.reserved_sectors) * bpb.bytes_per_sector;
    if (fat_bytes == 0u || fat0 + fat_bytes > volume.size())
    {
        return {};
    }
    return std::span<uint8_t>{volume.data() + fat0, fat_bytes};
}

std::vector<int> cluster_owners(const std::vector<dir_entry>& entries, uint32_t max_cluster,
                                int skip)
{
    std::vector<int> owner(static_cast<size_t>(max_cluster) + 1u, -1);
    for (size_t i = 0; i < entries.size(); ++i)
    {
        if (static_cast<int>(i) == skip)
        {
            continue;
        }
        const dir_entry& e = entries[i];
        if (e.deleted)
        {
            continue;
        }
        const bool live = is_payload_file(e) ||
                          ((e.attributes & k_attr_directory) != 0u && e.name_83 != "." &&
                           e.name_83 != "..");
        if (!live)
        {
            continue;
        }
        for (uint16_t c : e.cluster_chain)
        {
            if (c <= max_cluster)
            {
                owner[c] = static_cast<int>(i);
            }
        }
    }
    return owner;
}

std::vector<uint16_t> collect_free(std::span<const uint8_t> fat, fat_kind kind,
                                   uint32_t max_cluster,
                                   const std::unordered_set<uint16_t>& forbidden)
{
    std::vector<uint16_t> out;
    for (uint32_t c = 2; c <= max_cluster; ++c)
    {
        const uint16_t u = static_cast<uint16_t>(c);
        if (forbidden.count(u) != 0u)
        {
            continue;
        }
        if (cluster_is_free(fat, kind, c))
        {
            out.push_back(u);
        }
    }
    return out;
}

/**
 * @brief Move a live file onto free clusters so a neighbour can grow sequentially.
 *
 * Order is copy payload → link dest FAT → write dirent → free the old chain.
 * Dest clusters were free; if link or dirent fails they are returned to free
 * and the old chain is left allocated so the neighbour is not left with a
 * stale first_cluster pointing at freed FAT.
 *
 * @param[in,out] volume      FAT volume bytes.
 * @param[in]     bpb         Valid BPB.
 * @param[in]     kind        FAT12 or FAT16.
 * @param[in,out] fat         FAT0 (mutable).
 * @param[in,out] e           Live file to move (chain and first_cluster updated).
 * @param[in]     max_cluster Inclusive last data cluster.
 * @param[in]     forbidden   Clusters the grower wants; dest must not use them.
 *
 * @retval true  File now occupies @c dest; old clusters are free.
 * @retval false No dest run, copy range error, or FAT/dirent write failed.
 */
bool relocate_payload(std::vector<uint8_t>& volume, const bpb_info& bpb, fat_kind kind,
                      std::span<uint8_t> fat, dir_entry& e, uint32_t max_cluster,
                      const std::unordered_set<uint16_t>& forbidden)
{
    if ((e.attributes & k_attr_directory) != 0u)
    {
        return false;
    }
    const size_t n = e.cluster_chain.size();
    if (n == 0u)
    {
        return true;
    }
    const std::vector<uint16_t> dest = [&]()
    {
        const std::vector<uint16_t> free = collect_free(fat, kind, max_cluster, forbidden);
        std::vector<uint16_t> run;
        /* Prefer a contiguous run so the moved file stays linear. */
        for (size_t i = 0; i < free.size(); ++i)
        {
            run.clear();
            run.push_back(free[i]);
            for (size_t j = i + 1u; j < free.size() && run.size() < n; ++j)
            {
                if (free[j] == static_cast<uint16_t>(run.back() + 1u))
                {
                    run.push_back(free[j]);
                }
                else
                {
                    break;
                }
            }
            if (run.size() >= n)
            {
                run.resize(n);
                return run;
            }
        }
        if (free.size() >= n)
        {
            return std::vector<uint16_t>(free.begin(), free.begin() + static_cast<std::ptrdiff_t>(n));
        }
        return std::vector<uint16_t>{};
    }();
    if (dest.size() < n)
    {
        return false;
    }

    const uint32_t cluster_bytes =
        static_cast<uint32_t>(bpb.bytes_per_sector) * bpb.sectors_per_cluster;
    for (size_t i = 0; i < n; ++i)
    {
        const size_t src = cluster_offset(bpb, e.cluster_chain[i]);
        const size_t dst = cluster_offset(bpb, dest[i]);
        if (src >= volume.size() || dst >= volume.size())
        {
            return false;
        }
        const size_t room = std::min({static_cast<size_t>(cluster_bytes), volume.size() - src,
                                      volume.size() - dst});
        std::memmove(volume.data() + dst, volume.data() + src, room);
    }
    if (!link_chain(fat, kind, dest))
    {
        free_chain(fat, kind, dest);
        return false;
    }
    if (!write_dir_slot(volume, e.dir_slot_off, dest.front(), e.size))
    {
        free_chain(fat, kind, dest);
        return false;
    }
    free_chain(fat, kind, e.cluster_chain);
    e.cluster_chain = dest;
    e.first_cluster = dest.front();
    return true;
}

/**
 * @brief Free deleted-file clusters that are still allocated and not live-owned.
 *
 * Star Control TACTICS.PKG: a deleted dirent may carry the live file's
 * `cluster_chain` after `first_cluster` reuse. Those FAT slots stay allocated.
 *
 * @param[in,out] fat         FAT0.
 * @param[in]     kind        FAT12 or FAT16.
 * @param[in,out] entries     Directory listing; deleted chains are cleared.
 * @param[in]     max_cluster Inclusive last data cluster.
 */
void reclaim_deleted(std::span<uint8_t> fat, fat_kind kind, std::vector<dir_entry>& entries,
                     uint32_t max_cluster)
{
    const std::vector<int> owners = cluster_owners(entries, max_cluster, /*skip=*/-1);
    for (dir_entry& e : entries)
    {
        if (!e.deleted || !is_payload_file(e) || e.cluster_chain.empty())
        {
            continue;
        }
        std::vector<uint16_t> orphan;
        orphan.reserve(e.cluster_chain.size());
        for (uint16_t c : e.cluster_chain)
        {
            if (c > max_cluster)
            {
                continue;
            }
            if (owners[static_cast<size_t>(c)] >= 0)
            {
                continue;
            }
            if (!cluster_is_allocated(fat, kind, c))
            {
                continue;
            }
            orphan.push_back(c);
        }
        if (!orphan.empty())
        {
            free_chain(fat, kind, orphan);
        }
        e.cluster_chain.clear();
    }
}

bool replace_one(std::vector<uint8_t>& volume, const bpb_info& bpb, fat_kind kind,
                 std::vector<dir_entry>& entries, int target_i,
                 std::span<const uint8_t> payload, uint32_t max_cluster, std::string& err)
{
    dir_entry& target = entries[static_cast<size_t>(target_i)];
    const uint32_t cluster_bytes =
        static_cast<uint32_t>(bpb.bytes_per_sector) * bpb.sectors_per_cluster;
    if (cluster_bytes == 0u)
    {
        err = "invalid BPB cluster size";
        return false;
    }
    if (payload.size() > 0xFFFFFFFFu)
    {
        err = "host file is larger than a FAT12/16 size field";
        return false;
    }
    const uint32_t new_size = static_cast<uint32_t>(payload.size());
    const uint32_t need =
        (new_size == 0u) ? 0u : ((new_size + cluster_bytes - 1u) / cluster_bytes);

    std::span<uint8_t> fat = fat0_span(volume, bpb);
    if (fat.empty())
    {
        err = "FAT0 is truncated";
        return false;
    }

    std::string walk_notes;
    std::vector<uint16_t> chain =
        walk_chain(std::span<const uint8_t>(fat.data(), fat.size()), kind,
                   target.first_cluster, max_cluster, walk_notes);
    if (chain.empty() && target.first_cluster >= 2u)
    {
        chain = target.cluster_chain;
    }

    auto commit_dir = [&](uint16_t first, const std::vector<uint16_t>& used) -> bool
    {
        if (!write_dir_slot(volume, target.dir_slot_off, first, new_size))
        {
            err = "directory slot is out of range";
            return false;
        }
        target.first_cluster = first;
        target.size = new_size;
        target.cluster_chain = used;
        sync_fat_copies(volume, bpb);
        return true;
    };

    if (need == 0u)
    {
        free_chain(fat, kind, chain);
        return commit_dir(0, {});
    }

    if (need <= chain.size())
    {
        std::vector<uint16_t> keep(chain.begin(),
                                   chain.begin() + static_cast<std::ptrdiff_t>(need));
        std::vector<uint16_t> tail(chain.begin() + static_cast<std::ptrdiff_t>(need),
                                   chain.end());
        if (!write_chain_payload(volume, bpb, keep, payload))
        {
            err = "could not write payload into existing clusters";
            return false;
        }
        if (!link_chain(fat, kind, keep))
        {
            err = "could not update FAT chain";
            return false;
        }
        free_chain(fat, kind, tail);
        return commit_dir(keep.front(), keep);
    }

    /* Grow: keep the existing chain and append extra clusters. */
    const uint32_t extra = need - static_cast<uint32_t>(chain.size());
    std::unordered_set<uint16_t> reserved(chain.begin(), chain.end());

    std::vector<uint16_t> wanted;
    if (!chain.empty())
    {
        uint32_t cursor = static_cast<uint32_t>(chain.back()) + 1u;
        while (wanted.size() < extra && cursor <= max_cluster)
        {
            wanted.push_back(static_cast<uint16_t>(cursor));
            ++cursor;
        }

        const std::vector<int> owners = cluster_owners(entries, max_cluster, target_i);
        std::unordered_set<int> move_idx;
        for (uint16_t c : wanted)
        {
            if (c > max_cluster)
            {
                continue;
            }
            const int o = owners[c];
            if (o < 0)
            {
                continue;
            }
            const dir_entry& other = entries[static_cast<size_t>(o)];
            if ((other.attributes & k_attr_directory) != 0u)
            {
                continue;
            }
            move_idx.insert(o);
        }

        std::unordered_set<uint16_t> forbidden = reserved;
        for (uint16_t c : wanted)
        {
            forbidden.insert(c);
        }
        for (int o : move_idx)
        {
            dir_entry& other = entries[static_cast<size_t>(o)];
            if (!relocate_payload(volume, bpb, kind, fat, other, max_cluster, forbidden))
            {
                err = "could not relocate '" + other.name_83 + "' to grow the file";
                return false;
            }
        }
    }

    std::vector<uint16_t> add;
    auto take_free = [&]()
    {
        add.clear();
        for (uint16_t c : wanted)
        {
            if (add.size() >= extra)
            {
                break;
            }
            if (cluster_is_free(fat, kind, c) && reserved.count(c) == 0u)
            {
                add.push_back(c);
            }
        }
        if (add.size() >= extra)
        {
            add.resize(extra);
            return;
        }
        const std::unordered_set<uint16_t> skip = reserved;
        const std::vector<uint16_t> free = collect_free(fat, kind, max_cluster, skip);
        for (uint16_t c : free)
        {
            if (add.size() >= extra)
            {
                break;
            }
            bool already = false;
            for (uint16_t x : add)
            {
                if (x == c)
                {
                    already = true;
                    break;
                }
            }
            if (!already)
            {
                add.push_back(c);
            }
        }
    };

    take_free();
    if (add.size() < extra)
    {
        reclaim_deleted(fat, kind, entries, max_cluster);
        take_free();
    }
    if (add.size() < extra)
    {
        err = "not enough free clusters to grow the file";
        return false;
    }

    std::vector<uint16_t> neu = chain;
    neu.insert(neu.end(), add.begin(), add.end());
    if (!chain.empty())
    {
        if (!fat_set(fat, kind, chain.back(), add.front()))
        {
            err = "could not extend FAT chain";
            return false;
        }
    }
    if (!link_chain(fat, kind, add))
    {
        err = "could not link new FAT clusters";
        return false;
    }
    /* link_chain on `add` sets EOC on the last extra; if `chain` was empty the
       whole file is `add`. If `chain` was non-empty, fat_set already pointed
       the old tail at add.front() and link_chain chained the extras. */
    if (!write_chain_payload(volume, bpb, neu, payload))
    {
        err = "could not write grown payload";
        return false;
    }
    return commit_dir(neu.front(), neu);
}

std::vector<uint8_t> read_host(const std::filesystem::path& path, std::string& err)
{
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in)
    {
        err = "cannot read '" + path.string() + "'";
        return {};
    }
    const auto end = in.tellg();
    if (end < 0)
    {
        err = "cannot size '" + path.string() + "'";
        return {};
    }
    const size_t n = static_cast<size_t>(end);
    in.seekg(0, std::ios::beg);
    std::vector<uint8_t> bytes(n);
    if (n != 0u)
    {
        in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(n));
        if (!in)
        {
            err = "short read '" + path.string() + "'";
            return {};
        }
    }
    return bytes;
}

int find_live(const std::vector<dir_entry>& entries, const std::string& want_lower,
              std::string& err)
{
    int found = -1;
    int hits = 0;
    for (size_t i = 0; i < entries.size(); ++i)
    {
        const dir_entry& e = entries[i];
        if (!is_payload_file(e) || e.deleted)
        {
            continue;
        }
        if (ascii_lower(e.name_83) != want_lower)
        {
            continue;
        }
        ++hits;
        found = static_cast<int>(i);
    }
    if (hits == 0)
    {
        err = "no live file named '" + want_lower + "' in the image";
        return -1;
    }
    if (hits > 1)
    {
        err = "multiple live files named '" + want_lower + "'";
        return -1;
    }
    return found;
}

} /* namespace */

int update_files(analysis& a, const update_options& opt, std::ostream& err)
{
    if (!opt.enabled)
    {
        return 0;
    }
    if (a.flux.present && a.flux.format_name == "86BOX 86F")
    {
        err << "dumpfloppy: cannot update .86f flux images in this version\n";
        return -1;
    }
    if (!a.bpb.looks_valid ||
        (a.kind != fat_kind::fat12 && a.kind != fat_kind::fat16))
    {
        err << "dumpfloppy: image has no FAT12/16 volume to update\n";
        return -1;
    }
    if (opt.hosts.empty())
    {
        err << "dumpfloppy: --update requires a host file\n";
        return -1;
    }

    std::vector<uint8_t>& volume = volume_bytes_mut(a);
    const std::vector<uint8_t> snapshot = volume;
    const std::vector<dir_entry> entries_snap = a.entries;
    const uint32_t max_cluster = a.fat.max_cluster;

    int replaced = 0;
    for (const std::filesystem::path& host : opt.hosts)
    {
        std::string io_err;
        const std::vector<uint8_t> payload = read_host(host, io_err);
        if (!io_err.empty())
        {
            volume = snapshot;
            a.entries = entries_snap;
            err << "dumpfloppy: " << io_err << '\n';
            return -1;
        }
        const std::string want = ascii_lower(host.filename().string());
        std::string find_err;
        const int idx = find_live(a.entries, want, find_err);
        if (idx < 0)
        {
            volume = snapshot;
            a.entries = entries_snap;
            err << "dumpfloppy: " << find_err << '\n';
            return -1;
        }
        std::string rep_err;
        if (!replace_one(volume, a.bpb, a.kind, a.entries, idx, payload, max_cluster,
                         rep_err))
        {
            volume = snapshot;
            a.entries = entries_snap;
            err << "dumpfloppy: " << rep_err << '\n';
            return -1;
        }
        ++replaced;
    }
    return replaced;
}

} /* namespace dumpfloppy */
