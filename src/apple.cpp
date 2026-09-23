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
    case 0x04:
        return "TXT";
    case 0x06:
        return "BIN";
    case 0x0F:
        return "DIR";
    case 0x19:
        return "ADB";
    case 0x1A:
        return "AWP";
    case 0x1B:
        return "ASP";
    case 0xFC:
        return "BAS";
    case 0xFD:
        return "VAR";
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
    if (h[0x1F] != 0x27u || h[0x20] != 0x0Du)
    {
        return disk;
    }

    disk.present = true;
    disk.fs = apple_fs::prodos;
    disk.fs_name = "ProDOS";
    disk.volume_name.assign(reinterpret_cast<const char*>(h + 1), namelen);
    disk.total_blocks = le16(key, 4u + 0x25u);
    disk.volume.assign(data.begin(), data.end());

    const uint8_t entry_len = h[0x1F];
    const uint8_t per_block = h[0x20];
    uint16_t blk = 2;
    int hops = 0;
    while (blk != 0u && hops < 64)
    {
        ++hops;
        const auto block = prodos_block(disk.volume, blk);
        if (block.size() < k_apple_prodos_block)
        {
            break;
        }
        const uint16_t next = le16(block, 2);
        const int first = (blk == 2u) ? 1 : 0;
        for (int i = first; i < static_cast<int>(per_block); ++i)
        {
            const std::size_t e =
                4u + static_cast<std::size_t>(i) * entry_len;
            if (e + entry_len > block.size())
            {
                break;
            }
            const uint8_t st = static_cast<uint8_t>(block[e] >> 4);
            const uint8_t nlen = static_cast<uint8_t>(block[e] & 0x0Fu);
            if (st == 0u && nlen == 0u)
            {
                continue;
            }
            apple_file f{};
            f.deleted = (st == 0u);
            f.storage = st;
            if (nlen >= 1u && nlen <= 15u)
            {
                f.name.assign(reinterpret_cast<const char*>(block.data() + e + 1),
                              nlen);
            }
            if (f.deleted && !f.name.empty())
            {
                f.name[0] = '?';
            }
            f.type_byte = block[e + 0x10u];
            f.type_name = (st == 0x0Du) ? "DIR" : prodos_type_name(f.type_byte);
            f.key = le16(block, e + 0x11u);
            f.byte_size = le24(block, e + 0x15u);
            disk.entries.push_back(std::move(f));
        }
        if (next == blk)
        {
            break;
        }
        blk = next;
    }
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

std::vector<uint8_t> read_prodos(const apple_disk& disk, const apple_file& file)
{
    if (file.storage == 0x0Du || file.key == 0u)
    {
        return {};
    }
    auto take_block = [&](uint16_t blk) -> std::span<const uint8_t>
    { return prodos_block(disk.volume, blk); };

    std::vector<uint8_t> out;
    if (file.storage == 0x01u)
    {
        const auto b = take_block(file.key);
        out.assign(b.begin(), b.end());
    }
    else if (file.storage == 0x02u)
    {
        const auto idx = take_block(file.key);
        if (idx.size() < k_apple_prodos_block)
        {
            return {};
        }
        for (int i = 0; i < 256; ++i)
        {
            const uint16_t blk = static_cast<uint16_t>(
                static_cast<unsigned>(idx[static_cast<std::size_t>(i)]) |
                (static_cast<unsigned>(
                     idx[static_cast<std::size_t>(i) + 256u])
                 << 8));
            if (blk == 0u)
            {
                continue;
            }
            const auto b = take_block(blk);
            out.insert(out.end(), b.begin(), b.end());
        }
    }
    else
    {
        return {};
    }
    if (file.byte_size < out.size())
    {
        out.resize(file.byte_size);
    }
    return out;
}

} /* namespace */

apple_disk parse_apple(std::span<const uint8_t> data)
{
    apple_disk dos = parse_dos33(data);
    if (dos.present)
    {
        return dos;
    }
    return parse_prodos(data);
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

std::vector<uint8_t> read_apple_file(const apple_disk& disk, const apple_file& file)
{
    if (disk.fs == apple_fs::dos33)
    {
        return read_dos33(disk, file);
    }
    if (disk.fs == apple_fs::prodos)
    {
        return read_prodos(disk, file);
    }
    return {};
}

} /* namespace dumpfloppy */
