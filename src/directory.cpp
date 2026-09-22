/**
 * @file directory.cpp
 * @brief 8.3 + LFN directory parser with deleted-entry recovery.
 */
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/bpb.hpp"
#include "dumpfloppy/util.hpp"

#include <algorithm>
#include <unordered_set>

namespace dumpfloppy
{
namespace
{

std::string ucs2_to_utf8(const uint16_t* units, size_t count)
{
    std::string out;
    for (size_t i = 0; i < count; ++i)
    {
        const uint16_t u = units[i];
        if (u == 0u || u == 0xFFFFu)
        {
            break;
        }
        if (u < 0x80u)
        {
            out.push_back(static_cast<char>(u));
        }
        else if (u < 0x800u)
        {
            out.push_back(static_cast<char>(0xC0u | (u >> 6)));
            out.push_back(static_cast<char>(0x80u | (u & 0x3Fu)));
        }
        else
        {
            out.push_back(static_cast<char>(0xE0u | (u >> 12)));
            out.push_back(static_cast<char>(0x80u | ((u >> 6) & 0x3Fu)));
            out.push_back(static_cast<char>(0x80u | (u & 0x3Fu)));
        }
    }
    return out;
}

std::string lfn_from_slot(std::span<const uint8_t> e)
{
    uint16_t units[13] = {};
    size_t n = 0;
    auto take = [&](size_t off, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            units[n++] = read_le16(e, off + i * 2u);
        }
    };
    take(1, 5);
    take(14, 6);
    take(28, 2);
    return ucs2_to_utf8(units, 13);
}

std::string join_path(const std::string& dir, const std::string& name)
{
    if (dir.empty() || dir == "\\")
    {
        return std::string("\\") + name;
    }
    return dir + "\\" + name;
}

void parse_dir_bytes(std::span<const uint8_t> image, const bpb_info& bpb,
                     fat_kind kind, std::span<const uint8_t> fat,
                     std::span<const uint8_t> dir_bytes, const std::string& dir_path,
                     std::vector<dir_entry>& out,
                     std::unordered_set<uint16_t>& visited_dirs);

void parse_one_slot(std::span<const uint8_t> image, const bpb_info& bpb,
                    fat_kind kind, std::span<const uint8_t> fat,
                    std::span<const uint8_t> slot, const std::string& dir_path,
                    std::string& pending_lfn, bool after_term,
                    std::vector<dir_entry>& out,
                    std::unordered_set<uint16_t>& visited_dirs)
{
    const uint8_t first = slot[0];
    const uint8_t attr = slot[11];
    const bool deleted = (first == k_dir_deleted);

    if ((attr & k_attr_lfn) == k_attr_lfn && attr != 0x00)
    {
        const std::string piece = lfn_from_slot(slot);
        if (deleted)
        {
            /* Keep going; a deleted LFN still names the following 8.3. */
        }
        if ((first & 0x40u) != 0u && first != k_dir_deleted)
        {
            pending_lfn = piece;
        }
        else
        {
            pending_lfn = piece + pending_lfn;
        }
        return;
    }

    dir_entry e{};
    e.deleted = deleted;
    e.after_terminator = after_term;
    e.attributes = attr;
    e.name_83 = format_name_83(slot.data(), deleted);
    e.lfn = pending_lfn;
    pending_lfn.clear();
    e.path = join_path(dir_path, e.lfn.empty() ? e.name_83 : e.lfn);
    e.nt_reserved = slot[12];
    e.create_tenth = slot[13];
    e.create_time = read_le16(slot, 14);
    e.create_date = read_le16(slot, 16);
    e.access_date = read_le16(slot, 18);
    e.write_time = read_le16(slot, 22);
    e.write_date = read_le16(slot, 24);
    const uint16_t cluster_hi = read_le16(slot, 20);
    e.first_cluster = read_le16(slot, 26);
    e.size = read_le32(slot, 28);
    if (cluster_hi != 0u)
    {
        e.notes = "FAT32 cluster high word is non-zero on a FAT12/16 volume";
    }

    const bool is_dot = (e.name_83 == "." || e.name_83 == "..");
    const uint32_t max_cluster = [&]()
    {
        const uint32_t n = data_cluster_count(bpb);
        return (n == 0u) ? 1u : (1u + n);
    }();

    if ((attr & k_attr_volume) != 0u && (attr & k_attr_directory) == 0u)
    {
        e.notes = "volume label";
        out.push_back(std::move(e));
        return;
    }

    if (!is_dot && e.first_cluster >= 2u)
    {
        std::string chain_notes;
        e.cluster_chain = walk_chain(fat, kind, e.first_cluster, max_cluster, chain_notes);
        if (!chain_notes.empty())
        {
            e.notes = chain_notes;
        }
        const size_t off = cluster_offset(bpb, e.first_cluster);
        if (off < image.size())
        {
            const size_t n = std::min<size_t>(16u, image.size() - off);
            e.magic = sniff_magic(std::span<const uint8_t>{image.data() + off, n});
        }
        const uint32_t cluster_bytes =
            static_cast<uint32_t>(bpb.bytes_per_sector) * bpb.sectors_per_cluster;
        if ((attr & k_attr_directory) == 0u && cluster_bytes > 0u &&
            !e.cluster_chain.empty())
        {
            const uint64_t cap =
                static_cast<uint64_t>(e.cluster_chain.size()) * cluster_bytes;
            if (e.size > cap)
            {
                if (!e.notes.empty())
                {
                    e.notes += "; ";
                }
                e.notes += "size larger than cluster chain";
            }
        }
    }

    out.push_back(e);

    if ((attr & k_attr_directory) != 0u && !is_dot && !deleted &&
        e.first_cluster >= 2u)
    {
        if (!visited_dirs.insert(e.first_cluster).second)
        {
            return;
        }
        std::string dummy;
        const std::vector<uint16_t> chain =
            walk_chain(fat, kind, e.first_cluster, max_cluster, dummy);
        std::vector<uint8_t> sub;
        const uint32_t cluster_bytes =
            static_cast<uint32_t>(bpb.bytes_per_sector) * bpb.sectors_per_cluster;
        sub.reserve(chain.size() * cluster_bytes);
        for (uint16_t cl : chain)
        {
            const size_t off = cluster_offset(bpb, cl);
            if (off >= image.size())
            {
                break;
            }
            const size_t n = std::min<size_t>(cluster_bytes, image.size() - off);
            sub.insert(sub.end(), image.data() + off, image.data() + off + n);
        }
        parse_dir_bytes(image, bpb, kind, fat, sub, e.path, out, visited_dirs);
    }
}

void parse_dir_bytes(std::span<const uint8_t> image, const bpb_info& bpb,
                     fat_kind kind, std::span<const uint8_t> fat,
                     std::span<const uint8_t> dir_bytes, const std::string& dir_path,
                     std::vector<dir_entry>& out,
                     std::unordered_set<uint16_t>& visited_dirs)
{
    const size_t slots = dir_bytes.size() / 32u;
    bool seen_end = false;
    std::string pending_lfn;
    for (size_t i = 0; i < slots; ++i)
    {
        const std::span<const uint8_t> slot{dir_bytes.data() + i * 32u, 32u};
        const uint8_t first = slot[0];
        if (first == k_dir_end)
        {
            seen_end = true;
            pending_lfn.clear();
            continue;
        }
        parse_one_slot(image, bpb, kind, fat, slot, dir_path, pending_lfn, seen_end,
                       out, visited_dirs);
    }
}

} /* namespace */

std::string format_name_83(const uint8_t name[11], bool deleted)
{
    char base[8];
    char ext[3];
    for (int i = 0; i < 8; ++i)
    {
        uint8_t b = name[static_cast<size_t>(i)];
        if (i == 0 && deleted)
        {
            b = static_cast<uint8_t>('?');
        }
        else if (i == 0 && b == k_dir_kanji_e5)
        {
            b = k_dir_deleted;
        }
        base[static_cast<size_t>(i)] = static_cast<char>(b);
    }
    for (int i = 0; i < 3; ++i)
    {
        ext[static_cast<size_t>(i)] = static_cast<char>(name[static_cast<size_t>(8 + i)]);
    }

    auto trim = [](const char* p, int n)
    {
        while (n > 0 && p[n - 1] == ' ')
        {
            --n;
        }
        return std::string(p, static_cast<size_t>(n));
    };

    const std::string b = trim(base, 8);
    const std::string e = trim(ext, 3);
    if (e.empty())
    {
        return b.empty() ? "?" : b;
    }
    return b + "." + e;
}

std::string format_attributes(uint8_t attr)
{
    std::string s = "------";
    if ((attr & k_attr_read_only) != 0u)
    {
        s[0] = 'R';
    }
    if ((attr & k_attr_hidden) != 0u)
    {
        s[1] = 'H';
    }
    if ((attr & k_attr_system) != 0u)
    {
        s[2] = 'S';
    }
    if ((attr & k_attr_volume) != 0u)
    {
        s[3] = 'V';
    }
    if ((attr & k_attr_directory) != 0u)
    {
        s[4] = 'D';
    }
    if ((attr & k_attr_archive) != 0u)
    {
        s[5] = 'A';
    }
    return s;
}

std::string sniff_magic(std::span<const uint8_t> head)
{
    if (head.size() >= 2u && head[0] == 'M' && head[1] == 'Z')
    {
        return "MZ EXE";
    }
    if (head.size() >= 2u && head[0] == 'Z' && head[1] == 'M')
    {
        return "ZM EXE";
    }
    if (head.size() >= 2u && head[0] == 'P' && head[1] == 'K')
    {
        return "PK (zip/PKLITE)";
    }
    if (head.size() >= 4u && head[0] == 0x7Fu && head[1] == 'E' && head[2] == 'L' &&
        head[3] == 'F')
    {
        return "ELF (not DOS)";
    }
    if (head.size() >= 3u && head[0] == 0xCDu && head[1] == 0x20u)
    {
        return "COM (INT 20h)";
    }
    return {};
}

std::vector<dir_entry> list_directories(std::span<const uint8_t> image,
                                        const bpb_info& bpb, fat_kind kind,
                                        std::span<const uint8_t> fat)
{
    std::vector<dir_entry> out;
    if (!bpb.looks_valid || bpb.bytes_per_sector == 0u)
    {
        return out;
    }
    const size_t root_off =
        (static_cast<size_t>(bpb.reserved_sectors) +
         static_cast<size_t>(bpb.fat_count) * bpb.sectors_per_fat_16) *
        bpb.bytes_per_sector;
    const size_t root_bytes =
        static_cast<size_t>(bpb.root_entry_count) * 32u;
    if (root_off >= image.size())
    {
        return out;
    }
    const size_t n = std::min(root_bytes, image.size() - root_off);
    const std::span<const uint8_t> root{image.data() + root_off, n};
    std::unordered_set<uint16_t> visited;
    parse_dir_bytes(image, bpb, kind, fat, root, "\\", out, visited);
    return out;
}

} /* namespace dumpfloppy */
