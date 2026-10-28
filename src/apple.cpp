/**
 * @file apple.cpp
 * @brief DOS 3.3 VTOC/catalog and ProDOS volume-directory parse + extract.
 */
#include "dumpfloppy/apple.hpp"
#include "dumpfloppy/dos33_chain.h"

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

/**
 * @brief DOS 3.3 CATALOG type letters: T I A B S R, A2 / B2 for the "new A"
 *        ($20) and "new B" ($40) types, `*` in front when bit 7 (locked) is set.
 *
 * A type byte with no single known bit prints as `$XX` (low 7 bits).
 */
std::string dos_type_name(uint8_t type)
{
    const char* base = nullptr;
    switch (type & 0x7Fu)
    {
    case 0x00:
        base = "T";
        break;
    case 0x01:
        base = "I";
        break;
    case 0x02:
        base = "A";
        break;
    case 0x04:
        base = "B";
        break;
    case 0x08:
        base = "S";
        break;
    case 0x10:
        base = "R";
        break;
    case 0x20:
        base = "A2";
        break;
    case 0x40:
        base = "B2";
        break;
    default:
        break;
    }
    std::string out = (type & 0x80u) != 0u ? "*" : "";
    if (base != nullptr)
    {
        out += base;
    }
    else
    {
        static constexpr char k_hex[] = "0123456789ABCDEF";
        const unsigned v = type & 0x7Fu;
        out.push_back('$');
        out.push_back(k_hex[(v >> 4) & 0x0Fu]);
        out.push_back(k_hex[v & 0x0Fu]);
    }
    return out;
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

/** @brief DOS 3.3 T/S list: data sector pairs start at byte $0C, 122 per list. */
constexpr std::size_t k_dos_ts_pairs = 122u;

/** @brief Result of following one DOS 3.3 track/sector list chain. */
struct dos33_walk
{
    std::vector<uint8_t> bytes{}; /**< Data placed at its file offset; holes read as zeros. */
    std::size_t data_sectors = 0; /**< Data sectors actually present. */
    bool holes = false;           /**< A zero pair before the last data sector (random access). */
    unsigned bad_ptrs = 0;        /**< Data sector pointers outside the volume. */
    unsigned bad_lists = 0;       /**< T/S list pointers outside the volume. */
    bool loop = false;            /**< A T/S list sector was reached twice. */
    bool capped = false;          /**< File offsets past the volume size were dropped. */
};

/**
 * @brief Follow a T/S list chain from @p t / @p s.
 *
 * List k describes file sectors k×122 … k×122+121, so a zero pair is a hole
 * (random-access text files) and later data keeps its offset. The result
 * ends after the last present data sector (the T/S-list-based length).
 * Every list sector is visited at most once and output never exceeds the
 * volume size, so a looping or hostile chain cannot hang or amplify.
 */
dos33_walk walk_dos33_ts(const apple_disk& disk, uint8_t t, uint8_t s)
{
    dos33_walk w{};
    const uint8_t spt = disk.sectors_per_track;
    const std::size_t vol_sectors = disk.volume.size() / k_apple_dos_sector;
    if (spt == 0u || vol_sectors == 0u)
    {
        return w;
    }
    std::vector<bool> seen(vol_sectors, false);
    std::size_t list_index = 0;
    while (t != 0u)
    {
        const std::size_t idx = static_cast<std::size_t>(t) * spt + s;
        if (s >= spt || idx >= vol_sectors)
        {
            ++w.bad_lists;
            break;
        }
        if (seen[idx])
        {
            w.loop = true;
            break;
        }
        seen[idx] = true;
        const auto ts = dos_sector(disk.volume, t, s, spt);
        for (std::size_t i = 0; i < k_dos_ts_pairs; ++i)
        {
            const uint8_t dt = ts[0x0Cu + i * 2u];
            const uint8_t ds = ts[0x0Du + i * 2u];
            if (dt == 0u)
            {
                continue; /* track 0 never holds file data: a hole */
            }
            const std::size_t didx = static_cast<std::size_t>(dt) * spt + ds;
            if (ds >= spt || didx >= vol_sectors)
            {
                ++w.bad_ptrs;
                continue;
            }
            const std::size_t pos = list_index * k_dos_ts_pairs + i;
            const std::size_t end = (pos + 1u) * k_apple_dos_sector;
            if (end > disk.volume.size())
            {
                w.capped = true;
                continue;
            }
            if (w.bytes.size() < end)
            {
                w.bytes.resize(end, 0);
            }
            const auto sec = dos_sector(disk.volume, dt, ds, spt);
            std::memcpy(w.bytes.data() + pos * k_apple_dos_sector, sec.data(), sec.size());
            ++w.data_sectors;
        }
        ++list_index;
        t = ts[1];
        s = ts[2];
    }
    w.holes = w.data_sectors * k_apple_dos_sector < w.bytes.size();
    return w;
}

/**
 * @brief Set the real length of a DOS 3.3 file from its walked data.
 *
 * I and A files start with a 2-byte length, B files with a 2-byte load
 * address and a 2-byte length: the real size is header + length when it
 * fits in the data the T/S list holds. T files have no length field: a
 * sequential file ends at its first $00; one with holes in its T/S list is
 * random-access and keeps the T/S-list extent. Every other type (and a
 * header that does not fit) uses the T/S-list extent.
 */
void set_dos33_length(apple_file& f, const dos33_walk& w)
{
    const unsigned base = f.type_byte & 0x7Fu;
    const std::size_t have = w.bytes.size();
    f.dos_has_header = false;
    f.dos_header_len = 0;
    f.dos_load_addr = 0;
    f.byte_size = static_cast<uint32_t>(have);
    f.dos_len_source = have == 0u ? 0u : 2u;
    if (base == 0x00u && have != 0u && !w.holes)
    {
        const auto z = std::find(w.bytes.begin(), w.bytes.end(), uint8_t{0});
        f.byte_size = static_cast<uint32_t>(z - w.bytes.begin());
        f.dos_len_source = 3u;
        return;
    }
    const std::size_t hdr = (base == 0x01u || base == 0x02u) ? 2u : (base == 0x04u ? 4u : 0u);
    if (hdr == 0u || have < hdr)
    {
        return;
    }
    const std::span<const uint8_t> b(w.bytes);
    const uint16_t len = le16(b, hdr - 2u);
    if (hdr + len > have)
    {
        return;
    }
    f.dos_has_header = true;
    f.dos_header_len = len;
    f.dos_load_addr = hdr == 4u ? le16(b, 0) : 0u;
    f.byte_size = static_cast<uint32_t>(hdr + len);
    f.dos_len_source = 1u;
}

/**
 * @brief VTOC free-sector map lookup (16-sector DOS 3.3 only).
 *
 * Track t uses VTOC bytes $38+4t and $39+4t: bit 7 of the first byte is
 * sector 15 … bit 0 sector 8, bit 7 of the second is sector 7 … bit 0
 * sector 0. A set bit means free.
 */
bool dos33_sector_free(const apple_disk& disk, uint8_t t, uint8_t s)
{
    const auto vtoc = dos_sector(disk.volume, 17u, 0u, disk.sectors_per_track);
    const std::size_t at = 0x38u + static_cast<std::size_t>(t) * 4u + (s >= 8u ? 0u : 1u);
    if (vtoc.empty() || at >= vtoc.size() || s >= 16u)
    {
        return false;
    }
    return (vtoc[at] & (1u << (s & 7u))) != 0u;
}

} /* namespace */

dos33_undelete undelete_dos33(const apple_disk& disk, const apple_file& file)
{
    dos33_undelete r{};
    if (disk.fs != apple_fs::dos33 || !file.deleted)
    {
        return r;
    }
    const uint8_t spt = disk.sectors_per_track;
    const bool map_ok = spt == 16u;
    std::vector<std::string> why;
    uint8_t lt[DOS33_MAX_TS_LISTS]{};
    uint8_t ls[DOS33_MAX_TS_LISTS]{};
    std::size_t n = 0;
    const dos33_chain_status st =
        dos33_ts_chain(disk.volume.data(), disk.volume.size(), disk.tracks, spt, file.ts_track,
                       file.ts_sector, lt, ls, DOS33_MAX_TS_LISTS, &n);
    r.lists = static_cast<unsigned>(n);
    if (n == 0u)
    {
        r.status = dos33_recovery::unrecoverable;
        r.reason = "old T/S list T" + std::to_string(file.ts_track) + " S" +
                   std::to_string(file.ts_sector) + " is outside the volume";
        return r;
    }
    if (st == DOS33_CHAIN_BAD_LINK)
    {
        why.push_back("T/S list chain has a bad link after " + std::to_string(n) + " list(s)");
    }
    else if (st == DOS33_CHAIN_LOOP)
    {
        why.push_back("T/S list chain loops after " + std::to_string(n) + " list(s)");
    }
    else if (st == DOS33_CHAIN_TOO_LONG)
    {
        why.push_back("T/S list chain longer than " + std::to_string(DOS33_MAX_TS_LISTS) +
                      " lists");
    }
    dos33_walk w{};
    unsigned bad_ptrs = 0;
    for (std::size_t k = 0; k < n; ++k)
    {
        if (map_ok && !dos33_sector_free(disk, lt[k], ls[k]))
        {
            ++r.reused;
        }
        const auto ts = dos_sector(disk.volume, lt[k], ls[k], spt);
        for (std::size_t i = 0; i < k_dos_ts_pairs; ++i)
        {
            const uint8_t dt = ts[0x0Cu + i * 2u];
            const uint8_t ds = ts[0x0Du + i * 2u];
            if (dt == 0u)
            {
                continue;
            }
            if (dt >= disk.tracks || ds >= spt ||
                dos_sector(disk.volume, dt, ds, spt).empty())
            {
                ++bad_ptrs;
                continue;
            }
            const std::size_t pos = k * k_dos_ts_pairs + i;
            const std::size_t end = (pos + 1u) * k_apple_dos_sector;
            if (end > disk.volume.size())
            {
                w.capped = true;
                continue;
            }
            if (map_ok && !dos33_sector_free(disk, dt, ds))
            {
                ++r.reused;
            }
            if (w.bytes.size() < end)
            {
                w.bytes.resize(end, 0);
            }
            const auto sec = dos_sector(disk.volume, dt, ds, spt);
            std::memcpy(w.bytes.data() + pos * k_apple_dos_sector, sec.data(), sec.size());
            ++w.data_sectors;
        }
    }
    w.holes = w.data_sectors * k_apple_dos_sector < w.bytes.size();
    r.data_sectors = static_cast<unsigned>(w.data_sectors);
    if (r.reused != 0u)
    {
        why.push_back(std::to_string(r.reused) +
                      " sector(s) marked used again in the VTOC map (may hold a later file)");
    }
    if (bad_ptrs != 0u)
    {
        why.push_back(std::to_string(bad_ptrs) + " data pointer(s) outside the volume skipped");
    }
    if (w.capped)
    {
        why.push_back("offsets past the volume size dropped");
    }
    if (!map_ok)
    {
        why.push_back("free-sector map not checked (" + std::to_string(spt) +
                      " sectors per track)");
    }
    if (w.data_sectors == 0u)
    {
        r.status = dos33_recovery::unrecoverable;
        why.insert(why.begin(), "no data sectors left in the old T/S list");
    }
    else
    {
        r.status = why.empty() ? dos33_recovery::full : dos33_recovery::partial;
        apple_file sized = file;
        set_dos33_length(sized, w);
        if (sized.byte_size < w.bytes.size())
        {
            w.bytes.resize(sized.byte_size);
        }
        r.bytes = std::move(w.bytes);
    }
    for (std::size_t i = 0; i < why.size(); ++i)
    {
        r.reason += (i == 0u ? "" : "; ") + why[i];
    }
    return r;
}

const char* dos33_recovery_name(dos33_recovery r)
{
    switch (r)
    {
    case dos33_recovery::full:
        return "full";
    case dos33_recovery::partial:
        return "partial";
    case dos33_recovery::unrecoverable:
        return "unrecoverable";
    case dos33_recovery::none:
        break;
    }
    return "";
}

std::string apple_undelete_filename(const apple_file& file, dos33_recovery r)
{
    std::string stem = file.name.empty() ? std::string("FILE") : file.name;
    for (char& c : stem)
    {
        if (c == '/' || c == '\\' || c == '\0')
        {
            c = '_';
        }
    }
    return stem + (r == dos33_recovery::full ? ".undeleted" : ".partial");
}

namespace
{

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

        /* Catalog chain: every sector at most once (a loop cannot repeat entries). */
        std::vector<bool> cat_seen(static_cast<std::size_t>(tracks) * spt, false);
        while (cat_t != 0u && cat_t < tracks && cat_s < spt)
        {
            const std::size_t ci = static_cast<std::size_t>(cat_t) * spt + cat_s;
            if (cat_seen[ci])
            {
                disk.problems.push_back("catalog sector chain loops at T" +
                                        std::to_string(cat_t) + " S" + std::to_string(cat_s));
                break;
            }
            cat_seen[ci] = true;
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
                const uint8_t ts_t = cat[e];
                if (ts_t == 0u)
                {
                    continue; /* never used */
                }
                apple_file f{};
                /* DELETE copies the T/S list track into the last name byte
                 * ($20 in the entry) and writes $FF over the first byte. */
                f.deleted = (ts_t == 0xFFu);
                f.ts_track = f.deleted ? cat[e + 0x20u] : ts_t;
                f.dos_old_track = f.deleted ? cat[e + 0x20u] : uint8_t{0};
                f.ts_sector = cat[e + 1u];
                f.type_byte = cat[e + 2u];
                f.locked = (f.type_byte & 0x80u) != 0u;
                f.type_name = dos_type_name(f.type_byte);
                /* Name: $03-$20 live; $03-$1F when deleted ($20 is the old track). */
                f.name = strip_hi(cat.data() + e + 3u, f.deleted ? 29u : 30u);
                f.dos_sectors = le16(cat, e + 33u);
                f.key = static_cast<uint16_t>((static_cast<unsigned>(f.ts_track)
                                               << 8) |
                                              f.ts_sector);
                if (f.ts_track != 0u && f.ts_track < tracks && f.ts_sector < spt)
                {
                    set_dos33_length(f, walk_dos33_ts(disk, f.ts_track, f.ts_sector));
                }
                disk.entries.push_back(std::move(f));
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
std::vector<uint8_t> read_dos33(const apple_disk& disk, const apple_file& file,
                                apple_read_issues* issues)
{
    /* A deleted file's sectors may be reused: damage there is a note. */
    auto problem = [&](std::string msg)
    {
        if (issues != nullptr)
        {
            (file.deleted ? issues->notes : issues->errors).push_back(std::move(msg));
        }
    };
    if (file.ts_track == 0u || disk.sectors_per_track == 0u)
    {
        return {};
    }
    if (file.ts_track >= disk.tracks || file.ts_sector >= disk.sectors_per_track)
    {
        problem("T/S list T" + std::to_string(file.ts_track) + " S" +
                std::to_string(file.ts_sector) + " is outside the volume");
        return {};
    }
    dos33_walk w = walk_dos33_ts(disk, file.ts_track, file.ts_sector);
    if (w.bad_ptrs != 0u || w.bad_lists != 0u)
    {
        problem(std::to_string(w.bad_ptrs) + " data and " + std::to_string(w.bad_lists) +
                " T/S list pointer(s) are outside the volume; those bytes read as zeros");
    }
    if (w.loop)
    {
        problem("T/S list chain loops; stopped at the repeated list sector");
    }
    if (w.capped)
    {
        problem("T/S list addresses offsets past the volume size; output capped");
    }
    apple_file sized = file;
    set_dos33_length(sized, w);
    const unsigned base = file.type_byte & 0x7Fu;
    if (!sized.dos_has_header && (base == 0x01u || base == 0x02u || base == 0x04u) &&
        !w.bytes.empty() && issues != nullptr)
    {
        issues->notes.push_back("length word does not fit the " + std::to_string(w.bytes.size()) +
                                " bytes in the T/S list; size taken from the T/S list");
    }
    if (sized.byte_size < w.bytes.size())
    {
        w.bytes.resize(sized.byte_size);
    }
    return std::move(w.bytes);
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
    /* Deleted files carry the `?` marker on the host (FAT/TRD convention).
     * ProDOS already overwrote its first character; DOS 3.3 keeps the full
     * name in the catalog, so the marker is prefixed instead. */
    if (file.deleted && stem.front() != '?')
    {
        stem.insert(stem.begin(), '?');
    }
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
        return read_dos33(disk, file, issues);
    }
    if (disk.fs == apple_fs::prodos)
    {
        return read_prodos(disk, file, issues);
    }
    return {};
}

} /* namespace dumpfloppy */
