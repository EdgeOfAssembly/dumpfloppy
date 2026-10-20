/**
 * @file apple.cpp
 * @brief DOS 3.3 VTOC/catalog and ProDOS volume-directory parse + extract.
 */
#include "dumpfloppy/apple.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <vector>

namespace dumpfloppy
{
namespace
{

uint16_t le16(std::span<const uint8_t> d, std::size_t off)
{
    return static_cast<uint16_t>(static_cast<unsigned>(d[off]) |
                                 (static_cast<unsigned>(d[off + 1u]) << 8));
}

uint32_t le24(std::span<const uint8_t> d, std::size_t off)
{
    return static_cast<uint32_t>(d[off]) |
           (static_cast<uint32_t>(d[off + 1u]) << 8) |
           (static_cast<uint32_t>(d[off + 2u]) << 16);
}

std::string strip_hi(const uint8_t* p, std::size_t n)
{
    std::string out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        const char c = static_cast<char>(p[i] & 0x7Fu);
        if (c == 0)
        {
            break;
        }
        out.push_back(c);
    }
    while (!out.empty() && out.back() == ' ')
    {
        out.pop_back();
    }
    return out;
}

const char* dos_type_name(uint8_t type)
{
    switch (type & 0x7Fu)
    {
    case 0x00:
        return "TXT";
    case 0x01:
        return "INT";
    case 0x02:
        return "APP";
    case 0x04:
        return "BIN";
    case 0x08:
        return "S";
    case 0x10:
        return "REL";
    case 0x20:
        return "A";
    case 0x40:
        return "B";
    default:
        return "FILE";
    }
}

const char* prodos_type_name(uint8_t type)
{
    switch (type)
    {
    case 0x00:
        return "NON";
    case 0x01:
        return "BAD";
    case 0x04:
        return "TXT";
    case 0x06:
        return "BIN";
    case 0x08:
        return "FOT";
    case 0x0F:
        return "DIR";
    case 0x19:
        return "ADB";
    case 0x1A:
        return "AWP";
    case 0x1B:
        return "ASP";
    case 0x50:
        return "GWP";
    case 0x5A:
        return "CFG";
    case 0xB0:
        return "SRC";
    case 0xB1:
        return "OBJ";
    case 0xB2:
        return "LIB";
    case 0xB3:
        return "S16";
    case 0xB4:
        return "RTL";
    case 0xB5:
        return "EXE";
    case 0xB6:
        return "PIF";
    case 0xB7:
        return "TIF";
    case 0xB8:
        return "NDA";
    case 0xB9:
        return "CDA";
    case 0xBA:
        return "TOL";
    case 0xBB:
        return "DVR";
    case 0xBC:
        return "LDF";
    case 0xBD:
        return "FST";
    case 0xBF:
        return "DOC";
    case 0xC0:
        return "PNT";
    case 0xC1:
        return "PIC";
    case 0xC8:
        return "FON";
    case 0xC9:
        return "FND";
    case 0xCA:
        return "ICN";
    case 0xD5:
        return "MUS";
    case 0xD6:
        return "INS";
    case 0xD8:
        return "SND";
    case 0xE0:
        return "LBR";
    case 0xEF:
        return "PAS";
    case 0xF0:
        return "CMD";
    case 0xFA:
        return "INT";
    case 0xFB:
        return "IVR";
    case 0xFC:
        return "BAS";
    case 0xFD:
        return "VAR";
    case 0xFE:
        return "REL";
    case 0xFF:
        return "SYS";
    default:
        return "FILE";
    }
}

std::span<const uint8_t> dos_sector(std::span<const uint8_t> vol, uint8_t track,
                                    uint8_t sector, uint8_t spt)
{
    const std::size_t off = (static_cast<std::size_t>(track) * spt + sector) *
                            k_apple_dos_sector;
    if (off + k_apple_dos_sector > vol.size())
    {
        return {};
    }
    return vol.subspan(off, k_apple_dos_sector);
}

apple_disk parse_dos33(std::span<const uint8_t> data)
{
    apple_disk disk{};
    constexpr uint8_t k_vtoc_track = 17;
    auto try_spt = [&](uint8_t spt) -> bool
    {
        const auto vtoc = dos_sector(data, k_vtoc_track, 0, spt);
        if (vtoc.size() < 0x38u)
        {
            return false;
        }
        const uint8_t tracks = vtoc[0x34];
        const uint8_t vspt = vtoc[0x35];
        const unsigned bps = static_cast<unsigned>(vtoc[0x36]) |
                             (static_cast<unsigned>(vtoc[0x37]) << 8);
        if (tracks < 35u || tracks > 40u || vspt != spt || bps != 256u)
        {
            return false;
        }
        uint8_t cat_t = vtoc[1];
        uint8_t cat_s = vtoc[2];
        if (cat_t >= tracks || cat_s >= spt)
        {
            return false;
        }

        disk.present = true;
        disk.fs = apple_fs::dos33;
        disk.fs_name = "DOS 3.3";
        disk.volume_number = vtoc[6];
        disk.volume_name = "Volume " + std::to_string(disk.volume_number);
        disk.tracks = tracks;
        disk.sectors_per_track = spt;
        disk.volume.assign(data.begin(), data.end());

        int hops = 0;
        while (cat_t != 0u && hops < 32)
        {
            ++hops;
            const auto cat = dos_sector(disk.volume, cat_t, cat_s, spt);
            if (cat.size() < 256u)
            {
                break;
            }
            const uint8_t next_t = cat[1];
            const uint8_t next_s = cat[2];
            for (int i = 0; i < 7; ++i)
            {
                const std::size_t e = 0x0Bu + static_cast<std::size_t>(i) * 35u;
                if (e + 35u > cat.size())
                {
                    break;
                }
                const uint8_t ts_t = cat[e];
                if (ts_t == 0u)
                {
                    continue;
                }
                apple_file f{};
                f.deleted = (ts_t == 0xFFu);
                f.ts_track = ts_t;
                f.ts_sector = cat[e + 1u];
                f.type_byte = cat[e + 2u];
                f.locked = (f.type_byte & 0x80u) != 0u;
                f.type_name = dos_type_name(f.type_byte);
                f.name = strip_hi(cat.data() + e + 3u, 30u);
                if (f.deleted && !f.name.empty())
                {
                    f.name[0] = '?';
                }
                f.byte_size = static_cast<uint32_t>(le16(cat, e + 33u)) *
                              k_apple_dos_sector;
                f.key = static_cast<uint16_t>((static_cast<unsigned>(f.ts_track)
                                               << 8) |
                                              f.ts_sector);
                disk.entries.push_back(std::move(f));
            }
            if (next_t == cat_t && next_s == cat_s)
            {
                break;
            }
            cat_t = next_t;
            cat_s = next_s;
        }
        return true;
    };

    if (try_spt(16) || try_spt(13))
    {
        return disk;
    }
    return {};
}

std::span<const uint8_t> prodos_block(std::span<const uint8_t> vol, uint16_t blk)
{
    const std::size_t off = static_cast<std::size_t>(blk) * k_apple_prodos_block;
    if (off + k_apple_prodos_block > vol.size())
    {
        return {};
    }
    return vol.subspan(off, k_apple_prodos_block);
}

/** @brief Apply GS/OS lowercase bits (bit 15 set; bit 14 = char 0). */
void apply_case_bits(std::string& name, uint16_t bits)
{
    if ((bits & 0x8000u) == 0u)
    {
        return;
    }
    for (std::size_t i = 0; i < name.size() && i < 15u; ++i)
    {
        if ((bits & (0x4000u >> i)) != 0u && name[i] >= 'A' && name[i] <= 'Z')
        {
            name[i] = static_cast<char>(name[i] - 'A' + 'a');
        }
    }
}

/** @brief Directory entry geometry from a volume/subdirectory header. */
struct prodos_dir_geom
{
    uint8_t entry_len = 0;
    uint8_t per_block = 0;
};

bool prodos_geom_ok(uint8_t entry_len, uint8_t per_block)
{
    return entry_len >= 0x27u && per_block >= 1u &&
           4u + static_cast<unsigned>(entry_len) * per_block <= k_apple_prodos_block;
}

/**
 * @brief Walk one directory chain and recurse into subdirectories.
 *
 * Every block is visited at most once (visited set), and depth is
 * bounded, so a looping or cross-linked directory cannot hang.
 */
void walk_prodos_dir(apple_disk& disk, uint16_t key, const std::string& prefix,
                     prodos_dir_geom geom, std::vector<bool>& visited, int depth)
{
    if (depth > 32)
    {
        return;
    }
    uint16_t blk = key;
    bool first_block = true;
    while (blk != 0u)
    {
        if (blk >= visited.size() || visited[blk])
        {
            break;
        }
        visited[blk] = true;
        const auto block = prodos_block(disk.volume, blk);
        if (block.size() < k_apple_prodos_block)
        {
            break;
        }
        const uint16_t next = le16(block, 2);
        const int first = first_block ? 1 : 0;
        first_block = false;
        for (int i = first; i < static_cast<int>(geom.per_block); ++i)
        {
            const std::size_t e = 4u + static_cast<std::size_t>(i) * geom.entry_len;
            if (e + geom.entry_len > block.size())
            {
                break;
            }
            const uint8_t st = static_cast<uint8_t>(block[e] >> 4);
            const uint8_t nlen = static_cast<uint8_t>(block[e] & 0x0Fu);
            if (st == 0u && nlen == 0u)
            {
                continue;
            }
            if (st == 0x0Eu || st == 0x0Fu)
            {
                continue; /* stray header inside a chain */
            }
            apple_file f{};
            f.deleted = (st == 0u);
            f.storage = st;
            std::string leaf;
            if (nlen >= 1u)
            {
                leaf.assign(reinterpret_cast<const char*>(block.data() + e + 1), nlen);
            }
            apply_case_bits(leaf, le16(block, e + 0x1Cu));
            if (f.deleted && !leaf.empty())
            {
                leaf[0] = '?';
            }
            f.name = prefix + leaf;
            f.type_byte = block[e + 0x10u];
            f.type_name = (st == 0x0Du) ? "DIR" : prodos_type_name(f.type_byte);
            f.key = le16(block, e + 0x11u);
            f.byte_size = le24(block, e + 0x15u);
            f.locked = (block[e + 0x1Eu] & 0x02u) == 0u;
            f.aux_type = le16(block, e + 0x1Fu);
            if (st == 0x05u && !f.deleted)
            {
                /* Extended file: key block holds data and resource fork
                 * mini-entries at 0x000 and 0x100. */
                const auto ext = prodos_block(disk.volume, f.key);
                if (ext.size() == k_apple_prodos_block)
                {
                    apple_file data = f;
                    data.storage = static_cast<uint8_t>(ext[0] & 0x0Fu);
                    data.key = le16(ext, 1);
                    data.byte_size = le24(ext, 5);
                    data.extended = true;
                    apple_file rsrc = f;
                    rsrc.storage = static_cast<uint8_t>(ext[0x100] & 0x0Fu);
                    rsrc.key = le16(ext, 0x101);
                    rsrc.byte_size = le24(ext, 0x105);
                    rsrc.extended = true;
                    rsrc.resource_fork = true;
                    rsrc.name += ".rsrc";
                    disk.entries.push_back(std::move(data));
                    if (rsrc.byte_size != 0u)
                    {
                        disk.entries.push_back(std::move(rsrc));
                    }
                    continue;
                }
            }
            const bool recurse = (st == 0x0Du) && !f.deleted;
            const uint16_t sub_key = f.key;
            const std::string sub_prefix = f.name + "/";
            disk.entries.push_back(std::move(f));
            if (recurse)
            {
                const auto sk = prodos_block(disk.volume, sub_key);
                if (sk.size() == k_apple_prodos_block && (sk[4] >> 4) == 0x0Eu &&
                    prodos_geom_ok(sk[4u + 0x1Fu], sk[4u + 0x20u]))
                {
                    walk_prodos_dir(disk, sub_key, sub_prefix,
                                    {sk[4u + 0x1Fu], sk[4u + 0x20u]}, visited,
                                    depth + 1);
                }
            }
        }
        blk = next;
    }
}

apple_disk parse_prodos(std::span<const uint8_t> data)
{
    apple_disk disk{};
    if (data.size() < 3u * k_apple_prodos_block)
    {
        return disk;
    }
    const auto key = prodos_block(data, 2);
    if (key.size() < 0x2Bu)
    {
        return disk;
    }
    const uint8_t* h = key.data() + 4;
    if ((h[0] >> 4) != 0x0Fu)
    {
        return disk;
    }
    const uint8_t namelen = static_cast<uint8_t>(h[0] & 0x0Fu);
    if (namelen < 1u || namelen > 15u)
    {
        return disk;
    }
    /* Volume directory key block: prev pointer 0, entry length $27, 13 per block. */
    if (le16(key, 0) != 0u || h[0x1F] != 0x27u || h[0x20] != 0x0Du)
    {
        return disk;
    }

    disk.present = true;
    disk.fs = apple_fs::prodos;
    disk.fs_name = "ProDOS";
    disk.volume_name.assign(reinterpret_cast<const char*>(h + 1), namelen);
    apply_case_bits(disk.volume_name,
                    static_cast<uint16_t>(h[0x16] | (static_cast<unsigned>(h[0x17]) << 8)));
    disk.total_blocks = le16(key, 4u + 0x25u);
    disk.volume.assign(data.begin(), data.end());

    std::vector<bool> visited(disk.volume.size() / k_apple_prodos_block, false);
    walk_prodos_dir(disk, 2, std::string{}, {h[0x1F], h[0x20]}, visited, 0);
    return disk;
}
std::vector<uint8_t> read_dos33(const apple_disk& disk, const apple_file& file)
{
    if (file.ts_track == 0u || file.ts_track == 0xFFu ||
        disk.sectors_per_track == 0u)
    {
        return {};
    }
    std::vector<uint8_t> out;
    uint8_t t = file.ts_track;
    uint8_t s = file.ts_sector;
    int hops = 0;
    while (t != 0u && t != 0xFFu && hops < 64)
    {
        ++hops;
        const auto ts = dos_sector(disk.volume, t, s, disk.sectors_per_track);
        if (ts.size() < 256u)
        {
            break;
        }
        const uint8_t next_t = ts[1];
        const uint8_t next_s = ts[2];
        for (int i = 0; i < 122; ++i)
        {
            const std::size_t p = 0x0Cu + static_cast<std::size_t>(i) * 2u;
            const uint8_t dt = ts[p];
            const uint8_t ds = ts[p + 1u];
            if (dt == 0u)
            {
                continue;
            }
            const auto sec =
                dos_sector(disk.volume, dt, ds, disk.sectors_per_track);
            if (sec.empty())
            {
                continue;
            }
            out.insert(out.end(), sec.begin(), sec.end());
        }
        if (next_t == t && next_s == s)
        {
            break;
        }
        t = next_t;
        s = next_s;
    }
    if (file.byte_size > 0u && file.byte_size < out.size())
    {
        out.resize(file.byte_size);
    }
    return out;
}

/** @brief Largest EOF a ProDOS fork can address (24-bit). */
constexpr uint32_t k_prodos_max_eof = 0xFFFFFFu;

uint16_t index_ptr(std::span<const uint8_t> idx, std::size_t i)
{
    return static_cast<uint16_t>(static_cast<unsigned>(idx[i]) |
                                 (static_cast<unsigned>(idx[i + 256u]) << 8));
}

/**
 * @brief One fork read: tracks the end of real data and bad pointers.
 *
 * Output is never larger than the volume itself, and trailing sparse
 * space past the last allocated block is trimmed, so a hostile EOF
 * cannot amplify a small image into gigabytes on -x.
 */
struct prodos_reader
{
    const apple_disk& disk;
    std::vector<uint8_t>& out;
    std::size_t hi = 0;        /* end of last present data block */
    unsigned bad_ptrs = 0;     /* pointers past the end of the volume */
    unsigned bad_index = 0;    /* index blocks past the end */

    [[nodiscard]] std::size_t blocks() const
    {
        return disk.volume.size() / k_apple_prodos_block;
    }

    void data(uint16_t blk, std::size_t off)
    {
        if (blk == 0u || off >= out.size())
        {
            return;
        }
        if (blk >= blocks())
        {
            ++bad_ptrs;
            return;
        }
        const auto b = prodos_block(disk.volume, blk);
        const std::size_t n = std::min<std::size_t>(b.size(), out.size() - off);
        std::memcpy(out.data() + off, b.data(), n);
        hi = std::max(hi, off + n);
    }

    void index(uint16_t index_blk, std::size_t base)
    {
        if (index_blk == 0u || base >= out.size())
        {
            return;
        }
        if (index_blk >= blocks())
        {
            ++bad_index;
            return;
        }
        const auto idx = prodos_block(disk.volume, index_blk);
        for (std::size_t i = 0; i < 256u; ++i)
        {
            const std::size_t off = base + i * k_apple_prodos_block;
            if (off >= out.size())
            {
                break;
            }
            data(index_ptr(idx, i), off);
        }
    }
};

std::vector<uint8_t> read_prodos(const apple_disk& disk, const apple_file& file,
                                 apple_read_issues* issues)
{
    auto error = [&](std::string msg)
    {
        if (issues != nullptr)
        {
            issues->errors.push_back(std::move(msg));
        }
    };
    auto note = [&](std::string msg)
    {
        if (issues != nullptr)
        {
            issues->notes.push_back(std::move(msg));
        }
    };
    if (file.storage == 0x0Du || file.key == 0u)
    {
        return {};
    }
    if (file.storage < 1u || file.storage > 3u)
    {
        if (!file.deleted)
        {
            error("unsupported storage type " + std::to_string(file.storage));
        }
        return {};
    }
    const std::size_t vol_bytes = disk.volume.size();
    if (file.key >= vol_bytes / k_apple_prodos_block)
    {
        error("key block " + std::to_string(file.key) + " is past the end of the volume (" +
              std::to_string(vol_bytes / k_apple_prodos_block) + " blocks)");
        return {};
    }
    std::size_t want = std::min<std::size_t>(file.byte_size, k_prodos_max_eof);
    if (want > vol_bytes)
    {
        error("EOF " + std::to_string(file.byte_size) + " exceeds the volume size " +
              std::to_string(vol_bytes) + "; output capped");
        want = vol_bytes;
    }
    std::vector<uint8_t> out(want, 0);
    prodos_reader r{disk, out};
    if (file.storage == 0x01u)
    {
        r.data(file.key, 0);
    }
    else if (file.storage == 0x02u)
    {
        r.index(file.key, 0);
    }
    else
    {
        const auto master = prodos_block(disk.volume, file.key);
        constexpr std::size_t k_span = 256u * k_apple_prodos_block;
        for (std::size_t i = 0; i < 128u && i * k_span < out.size(); ++i)
        {
            r.index(index_ptr(master, i), i * k_span);
        }
    }
    if (r.bad_ptrs != 0u || r.bad_index != 0u)
    {
        error(std::to_string(r.bad_ptrs) + " data and " + std::to_string(r.bad_index) +
              " index block pointer(s) are out of range; those bytes read as zeros");
    }
    if (r.hi < out.size())
    {
        note("EOF " + std::to_string(file.byte_size) +
             " runs past the last allocated block; trimmed to " + std::to_string(r.hi));
        out.resize(r.hi);
    }
    return out;
}

} /* namespace */

apple_disk parse_apple_ordered(std::span<const uint8_t> data, apple_order order)
{
    auto remapped = [&]() -> apple_disk
    {
        const std::vector<uint8_t> po = apple_dos_order_to_prodos(data);
        if (po.empty())
        {
            return {};
        }
        apple_disk d = parse_prodos(po);
        if (d.present)
        {
            d.dos_order = true;
        }
        return d;
    };
    apple_disk dos = parse_dos33(data);
    if (dos.present)
    {
        return dos;
    }
    /* A 140K image's extension is not trusted: both orders are tried and the
     * one whose block 2 is a valid volume directory header wins. */
    const bool both = order == apple_order::unknown || data.size() == k_apple_dos33_140k;
    if (order == apple_order::dos)
    {
        apple_disk d = remapped();
        return (d.present || !both) ? d : parse_prodos(data);
    }
    apple_disk d = parse_prodos(data);
    if (d.present || !both)
    {
        return d;
    }
    return remapped();
}

apple_disk parse_apple(std::span<const uint8_t> data)
{
    apple_disk dos = parse_dos33(data);
    if (dos.present)
    {
        return dos;
    }
    return parse_prodos(data);
}

namespace
{

/** @brief po[s] = in[map[s]] for each 16-sector track. */
std::vector<uint8_t> remap_tracks(std::span<const uint8_t> in, const uint8_t (&map)[16])
{
    constexpr std::size_t k_track = 16u * k_apple_dos_sector;
    if (in.size() < k_track || (in.size() % k_track) != 0u)
    {
        return {};
    }
    std::vector<uint8_t> po(in.size(), 0);
    for (std::size_t base = 0; base < in.size(); base += k_track)
    {
        for (unsigned s = 0; s < 16u; ++s)
        {
            std::memcpy(po.data() + base + static_cast<std::size_t>(s) * k_apple_dos_sector,
                        in.data() + base + static_cast<std::size_t>(map[s]) * k_apple_dos_sector,
                        k_apple_dos_sector);
        }
    }
    return po;
}

} /* namespace */

std::vector<uint8_t> apple_dos_order_to_prodos(std::span<const uint8_t> dos_order)
{
    /* ProDOS-order sector s of a track is DOS-order sector k[s]: block 0 is
     * DOS sectors 0+14, block 7 is DOS 1+15 (Beneath Apple ProDOS). */
    static constexpr uint8_t k_prodos_from_dos[16] = {
        0, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 15};
    return remap_tracks(dos_order, k_prodos_from_dos);
}

std::vector<uint8_t> apple_physical_order_to_dos(std::span<const uint8_t> physical)
{
    /* DOS 3.3 logical sector L sits at physical sector 13L mod 15 (L<15), 15:
     * {0,13,11,9,7,5,3,1,14,12,10,8,6,4,2,15} (Beneath Apple DOS). */
    static constexpr uint8_t k_dos_from_phys[16] = {
        0, 13, 11, 9, 7, 5, 3, 1, 14, 12, 10, 8, 6, 4, 2, 15};
    return remap_tracks(physical, k_dos_from_phys);
}

std::vector<uint8_t> apple_physical_order_to_prodos(std::span<const uint8_t> physical)
{
    /* ProDOS logical sector s sits at physical sector 2s mod 15 (odd after 7). */
    static constexpr uint8_t k_prodos_from_phys[16] = {
        0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15};
    return remap_tracks(physical, k_prodos_from_phys);
}

std::string apple_host_filename(const apple_file& file)
{
    std::string stem = file.name.empty() ? std::string("FILE") : file.name;
    for (char& c : stem)
    {
        if (c == '/' || c == '\\' || c == '\0')
        {
            c = '_';
        }
    }
    return stem;
}

std::vector<uint8_t> read_apple_file(const apple_disk& disk, const apple_file& file,
                                     apple_read_issues* issues)
{
    if (disk.fs == apple_fs::dos33)
    {
        return read_dos33(disk, file);
    }
    if (disk.fs == apple_fs::prodos)
    {
        return read_prodos(disk, file, issues);
    }
    return {};
}

} /* namespace dumpfloppy */
