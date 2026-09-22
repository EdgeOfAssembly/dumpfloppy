/**
 * @file ibm_mfm.cpp
 * @brief HxC MFM track walk, IBM IDAM/DAM CRC, boot protection patterns.
 */
#include "dumpfloppy/ibm_mfm.hpp"
#include "dumpfloppy/bpb.hpp"
#include "dumpfloppy/image.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <vector>

namespace dumpfloppy
{
namespace
{

constexpr uint8_t k_sync[16] = {0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1};

/** IBM PC floppy SPT floor used for CHS assembly (160K is 8 spt). */
constexpr uint32_t k_min_spt = 8;
/** DMF 21 spt / 2.88M ED 36 spt; HLS IDs such as 241 stay outside this. */
constexpr uint32_t k_max_spt = 36;
/** Modal heads never exceed DS; a lone head=2 IDAM must not raise geometry. */
constexpr uint32_t k_max_heads = 2;

struct chs_policy
{
    uint32_t cyls = 0;
    uint32_t heads = 0;
    uint32_t spt = 0;
    uint32_t cap_sectors = 0; /**< 0 = only @ref k_max_image_bytes. */
};

bool is_std_512(const ibm_sector& s)
{
    return s.bytes == 512u && s.data.size() >= 512u;
}

bool geometry_from_bpb(std::span<const uint8_t> boot, chs_policy& g)
{
    const bpb_info b = parse_bpb(boot);
    if (!b.looks_valid || b.bytes_per_sector != 512u)
    {
        return false;
    }
    if (b.sectors_per_track < k_min_spt || b.sectors_per_track > k_max_spt)
    {
        return false;
    }
    if (b.head_count < 1u || b.head_count > k_max_heads)
    {
        return false;
    }
    g.spt = b.sectors_per_track;
    g.heads = b.head_count;
    g.cap_sectors = b.total_sectors;
    return true;
}

void geometry_from_modal(const std::vector<ibm_sector>& secs, chs_policy& g)
{
    struct track_occ
    {
        uint32_t key = 0;
        uint64_t mask = 0;
    };
    std::vector<track_occ> tracks;
    for (const ibm_sector& s : secs)
    {
        if (!is_std_512(s) || s.sector < 1u || s.sector > k_max_spt)
        {
            continue;
        }
        const uint32_t key =
            (static_cast<uint32_t>(s.cyl) << 8) | static_cast<uint32_t>(s.head);
        track_occ* row = nullptr;
        for (track_occ& t : tracks)
        {
            if (t.key == key)
            {
                row = &t;
                break;
            }
        }
        if (row == nullptr)
        {
            tracks.push_back(track_occ{key, 0});
            row = &tracks.back();
        }
        row->mask |= (uint64_t{1} << s.sector);
    }

    uint32_t hist[k_max_spt + 1u] = {};
    for (const track_occ& t : tracks)
    {
        uint32_t n = 0;
        for (uint32_t sec = 1; sec <= k_max_spt; ++sec)
        {
            if ((t.mask & (uint64_t{1} << sec)) == 0ull)
            {
                break;
            }
            n = sec;
        }
        if (n >= k_min_spt)
        {
            hist[n] += 1u;
        }
    }
    uint32_t best_n = 0;
    uint32_t best_c = 0;
    for (uint32_t n = k_min_spt; n <= k_max_spt; ++n)
    {
        if (hist[n] > best_c)
        {
            best_c = hist[n];
            best_n = n;
        }
    }
    g.spt = best_n;
    if (g.spt == 0u)
    {
        g.heads = 0;
        return;
    }

    uint32_t head_hits[256] = {};
    uint32_t n_in = 0;
    for (const ibm_sector& s : secs)
    {
        if (!is_std_512(s) || s.sector < 1u || s.sector > g.spt)
        {
            continue;
        }
        head_hits[s.head] += 1u;
        n_in += 1u;
    }
    const uint32_t min_hits = (n_in >= 16u) ? 2u : 1u;
    g.heads = 1u;
    if (head_hits[1] >= min_hits)
    {
        g.heads = 2u;
    }
}

void count_cylinders(const std::vector<ibm_sector>& secs, chs_policy& g)
{
    if (g.spt == 0u || g.heads == 0u)
    {
        g.cyls = 0;
        return;
    }
    uint32_t cyl_hits[256] = {};
    for (const ibm_sector& s : secs)
    {
        if (!is_std_512(s) || s.sector < 1u || s.sector > g.spt ||
            s.head >= g.heads)
        {
            continue;
        }
        cyl_hits[s.cyl] += 1u;
    }
    const uint32_t rich = (g.spt < 8u) ? g.spt : 8u;
    bool any_rich = false;
    uint32_t max_c = 0;
    for (uint32_t c = 0; c < 256u; ++c)
    {
        if (cyl_hits[c] >= rich)
        {
            any_rich = true;
            max_c = c;
        }
    }
    if (!any_rich)
    {
        max_c = 0;
        bool any = false;
        for (uint32_t c = 0; c < 256u; ++c)
        {
            if (cyl_hits[c] > 0u)
            {
                any = true;
                max_c = c;
            }
        }
        if (!any)
        {
            g.cyls = 0;
            return;
        }
    }
    g.cyls = max_c + 1u;
}

chs_policy choose_chs_policy(const flux_disk& d)
{
    chs_policy g{};
    if (!geometry_from_bpb(d.boot, g))
    {
        geometry_from_modal(d.sectors, g);
    }
    count_cylinders(d.sectors, g);
    if (g.cyls == 0u && g.spt >= k_min_spt && g.heads != 0u && !d.boot.empty())
    {
        g.cyls = 1u;
    }
    return g;
}

void assemble_chs(flux_disk& d, const chs_policy& g)
{
    d.chs_cyls = 0;
    d.chs_heads = 0;
    d.chs_spt = 0;
    d.assembled_chs.clear();
    if (g.spt < k_min_spt || g.heads == 0u || g.cyls == 0u)
    {
        return;
    }
    const uint64_t stride = static_cast<uint64_t>(g.heads) * g.spt;
    uint64_t nsec = static_cast<uint64_t>(g.cyls) * stride;
    uint64_t cap_n = k_max_image_bytes / 512u;
    if (g.cap_sectors != 0u)
    {
        cap_n = std::min(cap_n, static_cast<uint64_t>(g.cap_sectors));
    }
    if (nsec > cap_n)
    {
        nsec = cap_n;
    }
    if (nsec == 0u)
    {
        return;
    }
    d.chs_spt = g.spt;
    d.chs_heads = g.heads;
    const uint32_t cyls_fit = static_cast<uint32_t>(nsec / stride);
    d.chs_cyls = (cyls_fit != 0u) ? cyls_fit : 1u;
    d.assembled_chs.assign(static_cast<size_t>(nsec) * 512u, 0);
    const uint32_t heads = g.heads;
    const uint32_t spt = g.spt;
    for (const ibm_sector& s : d.sectors)
    {
        if (!is_std_512(s) || s.sector < 1u || s.sector > spt || s.head >= heads)
        {
            continue;
        }
        const size_t lba = (static_cast<size_t>(s.cyl) * heads + s.head) * spt +
                           (s.sector - 1u);
        const size_t off = lba * 512u;
        if (off + 512u > d.assembled_chs.size())
        {
            continue;
        }
        std::memcpy(d.assembled_chs.data() + off, s.data.data(), 512u);
    }
}

uint16_t crc16_ibm(const uint8_t* data, size_t n)
{
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < n; ++i)
    {
        crc = static_cast<uint16_t>(crc ^ static_cast<uint16_t>(data[i] << 8));
        for (int b = 0; b < 8; ++b)
        {
            if ((crc & 0x8000u) != 0u)
            {
                crc = static_cast<uint16_t>((crc << 1) ^ 0x1021u);
            }
            else
            {
                crc = static_cast<uint16_t>(crc << 1);
            }
        }
    }
    return crc;
}

void bits_from_bytes(std::span<const uint8_t> src, std::vector<uint8_t>& bits)
{
    bits.clear();
    bits.reserve(src.size() * 8u);
    for (uint8_t byte : src)
    {
        for (int i = 7; i >= 0; --i)
        {
            bits.push_back(static_cast<uint8_t>((byte >> i) & 1u));
        }
    }
}

bool sync3(const std::vector<uint8_t>& bits, size_t i)
{
    if (i + 48u > bits.size())
    {
        return false;
    }
    for (int s = 0; s < 3; ++s)
    {
        for (int b = 0; b < 16; ++b)
        {
            if (bits[i + static_cast<size_t>(s * 16 + b)] != k_sync[b])
            {
                return false;
            }
        }
    }
    return true;
}

bool decode_n(const std::vector<uint8_t>& bits, size_t& i, size_t nbytes,
              uint8_t* out)
{
    for (size_t n = 0; n < nbytes; ++n)
    {
        if (i + 16u > bits.size())
        {
            return false;
        }
        uint8_t val = 0;
        for (int b = 0; b < 8; ++b)
        {
            val = static_cast<uint8_t>((val << 1) | bits[i + 1u]);
            i += 2u;
        }
        out[n] = val;
    }
    return true;
}

void scan_track(std::span<const uint8_t> blob, std::vector<ibm_sector>& out,
                std::vector<uint8_t>* boot_out, size_t track_file_off)
{
    std::vector<uint8_t> bits;
    bits_from_bytes(blob, bits);
    ibm_sector pending{};
    bool have_id = false;
    size_t i = 0;
    while (i + 48u <= bits.size())
    {
        if (!sync3(bits, i))
        {
            ++i;
            continue;
        }
        size_t p = i + 48u;
        uint8_t mark = 0;
        if (!decode_n(bits, p, 1, &mark))
        {
            break;
        }
        if (mark == 0xFEu)
        {
            uint8_t rest[6] = {};
            if (!decode_n(bits, p, 6, rest))
            {
                break;
            }
            uint8_t rec[10] = {0xA1, 0xA1, 0xA1, 0xFE,
                               rest[0], rest[1], rest[2], rest[3], rest[4], rest[5]};
            pending = ibm_sector{};
            pending.cyl = rest[0];
            pending.head = rest[1];
            pending.sector = rest[2];
            pending.size_code = rest[3];
            pending.bytes = static_cast<uint16_t>(
                pending.size_code < 8u ? (128u << pending.size_code) : 512u);
            pending.idam_crc_ok = (crc16_ibm(rec, 10) == 0u);
            pending.track_file_off = track_file_off;
            pending.track_byte_len = blob.size();
            have_id = true;
            i = p;
            continue;
        }
        if ((mark == 0xFBu || mark == 0xF8u) && have_id)
        {
            const size_t nbytes = pending.bytes;
            pending.dam_mark = mark;
            pending.dam_bit_off = p;
            std::vector<uint8_t> payload(nbytes + 2u);
            if (!decode_n(bits, p, nbytes + 2u, payload.data()))
            {
                have_id = false;
                ++i;
                continue;
            }
            std::vector<uint8_t> rec(4u + nbytes + 2u);
            rec[0] = 0xA1;
            rec[1] = 0xA1;
            rec[2] = 0xA1;
            rec[3] = mark;
            std::memcpy(rec.data() + 4, payload.data(), nbytes + 2u);
            pending.has_dam = true;
            pending.dam_crc_ok = (crc16_ibm(rec.data(), rec.size()) == 0u);
            pending.data.assign(payload.begin(), payload.begin() + nbytes);
            if (boot_out != nullptr && pending.cyl == 0u && pending.head == 0u &&
                pending.sector == 1u && nbytes >= 512u)
            {
                boot_out->assign(payload.begin(), payload.begin() + 512);
            }
            out.push_back(pending);
            have_id = false;
            i = p;
            continue;
        }
        i = p;
    }
}

void fill_protection(flux_disk& d, uint32_t spt, uint32_t heads)
{
    d.protection.clear();
    unsigned long_n = 0;
    unsigned bad_dam = 0;
    unsigned bad_idam = 0;
    const uint32_t spt_lim = (spt >= k_min_spt) ? spt : k_max_spt;
    const uint32_t head_lim = (heads >= 1u) ? heads : k_max_heads;
    std::vector<uint32_t> seen_ids;
    auto id_key = [](const ibm_sector& s) -> uint32_t
    {
        return (static_cast<uint32_t>(s.cyl) << 16) |
               (static_cast<uint32_t>(s.head) << 8) | s.sector;
    };
    for (const ibm_sector& s : d.sectors)
    {
        const uint32_t key = id_key(s);
        bool dup = false;
        for (uint32_t k : seen_ids)
        {
            if (k == key)
            {
                dup = true;
                break;
            }
        }
        if (dup)
        {
            continue;
        }
        seen_ids.push_back(key);
        if (s.sector < 1u || s.sector > spt_lim || s.head >= head_lim)
        {
            char buf[96] = {};
            std::snprintf(buf, sizeof(buf),
                          "HLS-style nonstandard sector ID C%u:H%u:S%u (%u bytes)",
                          s.cyl, s.head, s.sector, s.bytes);
            d.protection.emplace_back(buf);
        }
        else if (s.size_code != 2u)
        {
            ++long_n;
            char buf[80] = {};
            std::snprintf(buf, sizeof(buf), "long sector C%u:H%u:S%u (%u bytes)",
                          s.cyl, s.head, s.sector, s.bytes);
            d.protection.emplace_back(buf);
        }
        if (s.has_dam && !s.dam_crc_ok)
        {
            ++bad_dam;
            char buf[80] = {};
            std::snprintf(buf, sizeof(buf),
                          "intentional bad DAM CRC C%u:H%u:S%u", s.cyl, s.head,
                          s.sector);
            d.protection.emplace_back(buf);
        }
        if (!s.idam_crc_ok)
        {
            ++bad_idam;
        }
    }
    if (bad_idam != 0u)
    {
        d.protection.emplace_back("bad IDAM CRC(s)");
    }
    (void)long_n;
    (void)bad_dam;
}

void write_mfm_bit(std::vector<uint8_t>& mfm, size_t abs_bit, uint8_t bit)
{
    const size_t byte_i = abs_bit / 8u;
    if (byte_i >= mfm.size())
    {
        return;
    }
    const unsigned shift = static_cast<unsigned>(7u - (abs_bit % 8u));
    if (bit != 0u)
    {
        mfm[byte_i] = static_cast<uint8_t>(mfm[byte_i] | static_cast<uint8_t>(1u << shift));
    }
    else
    {
        mfm[byte_i] =
            static_cast<uint8_t>(mfm[byte_i] & static_cast<uint8_t>(~(1u << shift)));
    }
}

bool encode_dam_payload(std::vector<uint8_t>& mfm, const ibm_sector& s,
                        std::span<const uint8_t> payload512)
{
    if (payload512.size() < 512u || s.track_byte_len == 0u)
    {
        return false;
    }
    constexpr size_t k_payload = 512u;
    constexpr size_t k_crc = 2u;
    const size_t nbits = (k_payload + k_crc) * 16u;
    const size_t track_bits = s.track_byte_len * 8u;
    if (s.dam_bit_off + nbits > track_bits)
    {
        return false;
    }
    if (s.track_file_off + s.track_byte_len > mfm.size())
    {
        return false;
    }

    uint8_t rec[4u + k_payload];
    rec[0] = 0xA1;
    rec[1] = 0xA1;
    rec[2] = 0xA1;
    rec[3] = s.dam_mark;
    std::memcpy(rec + 4, payload512.data(), k_payload);
    const uint16_t crc = crc16_ibm(rec, 4u + k_payload);

    uint8_t out[k_payload + k_crc];
    std::memcpy(out, payload512.data(), k_payload);
    out[k_payload] = static_cast<uint8_t>(crc >> 8);
    out[k_payload + 1u] = static_cast<uint8_t>(crc & 0xFFu);

    uint8_t prev = (s.dam_mark == 0xF8u) ? 0u : 1u;
    size_t bit = s.track_file_off * 8u + s.dam_bit_off;
    for (size_t i = 0; i < k_payload + k_crc; ++i)
    {
        for (int b = 7; b >= 0; --b)
        {
            const uint8_t data = static_cast<uint8_t>((out[i] >> b) & 1u);
            const uint8_t clock = (prev == 0u && data == 0u) ? 1u : 0u;
            write_mfm_bit(mfm, bit, clock);
            write_mfm_bit(mfm, bit + 1u, data);
            bit += 2u;
            prev = data;
        }
    }
    return true;
}

} /* namespace */

void finish_ibm_flux(flux_disk& disk)
{
    const chs_policy g = choose_chs_policy(disk);
    fill_protection(disk, g.spt, g.heads);
    assemble_chs(disk, g);
}

flux_disk decode_hxc_mfm(std::span<const uint8_t> file)
{
    flux_disk d{};
    if (file.size() < 19u || std::memcmp(file.data(), "HXCMFM", 6) != 0)
    {
        return d;
    }
    d.present = true;
    d.format_name = "HXC MFM";
    d.tracks = static_cast<uint32_t>(file[7] | (static_cast<uint32_t>(file[8]) << 8));
    d.sides = file[9];
    d.rpm = static_cast<uint32_t>(file[10] | (static_cast<uint32_t>(file[11]) << 8));
    d.bitrate_kbps =
        static_cast<uint32_t>(file[12] | (static_cast<uint32_t>(file[13]) << 8));
    const uint32_t list =
        static_cast<uint32_t>(file[15]) | (static_cast<uint32_t>(file[16]) << 8) |
        (static_cast<uint32_t>(file[17]) << 16) | (static_cast<uint32_t>(file[18]) << 24);
    const uint32_t n = d.tracks * (d.sides == 0u ? 1u : d.sides);
    for (uint32_t t = 0; t < n; ++t)
    {
        const size_t off = static_cast<size_t>(list) + static_cast<size_t>(t) * 11u;
        if (off + 11u > file.size())
        {
            d.note = "truncated HxC track list";
            break;
        }
        const uint32_t tsize = static_cast<uint32_t>(file[off + 3]) |
                               (static_cast<uint32_t>(file[off + 4]) << 8) |
                               (static_cast<uint32_t>(file[off + 5]) << 16) |
                               (static_cast<uint32_t>(file[off + 6]) << 24);
        const uint32_t toff = static_cast<uint32_t>(file[off + 7]) |
                              (static_cast<uint32_t>(file[off + 8]) << 8) |
                              (static_cast<uint32_t>(file[off + 9]) << 16) |
                              (static_cast<uint32_t>(file[off + 10]) << 24);
        if (static_cast<size_t>(toff) + tsize > file.size())
        {
            continue;
        }
        scan_track(file.subspan(toff, tsize), d.sectors, &d.boot, toff);
    }
    finish_ibm_flux(d);
    add_boot_protection(d, d.boot);
    if (d.boot.size() >= 512u)
    {
        const bool aa55 = d.boot[510] == 0x55u && d.boot[511] == 0xAAu;
        if (!aa55)
        {
            d.protection.emplace_back("no 55 AA boot signature");
        }
        if (d.boot[0] != 0xEBu && d.boot[0] != 0xE9u)
        {
            d.protection.emplace_back("custom booter (no DOS jump, no FAT)");
        }
    }
    return d;
}

flux_disk inspect_86f(std::span<const uint8_t> file)
{
    flux_disk d{};
    if (file.size() < 8u || std::memcmp(file.data(), "86BF", 4) != 0)
    {
        return d;
    }
    d.present = true;
    d.format_name = "86BOX 86F";
    const uint16_t flags =
        static_cast<uint16_t>(file[6] | (static_cast<uint16_t>(file[7]) << 8));
    d.sides = ((flags & 0x0008u) != 0u) ? 2u : 1u;
    char buf[96] = {};
    std::snprintf(buf, sizeof(buf),
                  "86F v%u.%u flags=0x%04X sides=%u DD hole=%u extra_bitcells=%s",
                  file[5], file[4], flags, d.sides, (flags >> 1) & 3u,
                  ((flags & 0x80u) != 0u) ? "yes" : "no");
    d.note = buf;
    d.protection.emplace_back(
        "flux image (86F); sector map needs HxC .mfm or an 86F decoder");
    return d;
}

bool patch_mfm_chs(std::vector<uint8_t>& mfm, const flux_disk& flux,
                   std::span<const uint8_t> new_chs)
{
    if (flux.assembled_chs.size() != new_chs.size() || flux.chs_spt == 0u ||
        flux.chs_heads == 0u)
    {
        return false;
    }
    const uint32_t heads = flux.chs_heads;
    const uint32_t spt = flux.chs_spt;
    for (const ibm_sector& s : flux.sectors)
    {
        if (s.bytes != 512u || s.sector < 1u || s.sector > spt || !s.has_dam ||
            s.data.size() < 512u)
        {
            continue;
        }
        const size_t lba =
            (static_cast<size_t>(s.cyl) * heads + s.head) * spt + (s.sector - 1u);
        const size_t off = lba * 512u;
        if (off + 512u > new_chs.size())
        {
            continue;
        }
        if (std::memcmp(s.data.data(), new_chs.data() + off, 512u) == 0)
        {
            continue;
        }
        if (!encode_dam_payload(mfm, s, std::span<const uint8_t>{new_chs.data() + off, 512u}))
        {
            return false;
        }
    }
    return true;
}

void add_boot_protection(flux_disk& disk, std::span<const uint8_t> boot)
{
    if (boot.size() < 8u)
    {
        return;
    }
    bool int13 = false;
    bool ah10 = false;
    bool int1e = false;
    for (size_t i = 0; i + 1u < boot.size(); ++i)
    {
        if (boot[i] == 0xCDu && boot[i + 1u] == 0x13u)
        {
            int13 = true;
        }
        if (boot[i] == 0x80u && i + 2u < boot.size() && boot[i + 1u] == 0xFCu &&
            boot[i + 2u] == 0x10u)
        {
            ah10 = true;
        }
        if (boot[i] == 0xBBu && i + 2u < boot.size() && boot[i + 1u] == 0x78u &&
            boot[i + 2u] == 0x00u)
        {
            int1e = true;
        }
    }
    if (int13 && ah10)
    {
        disk.protection.emplace_back("boot INT 13h waits for AH=10h (CRC error)");
    }
    if (int1e)
    {
        disk.protection.emplace_back("INT 1E floppy parameter table hooked");
    }
}

} /* namespace dumpfloppy */
