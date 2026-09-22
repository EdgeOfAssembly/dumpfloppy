/**
 * @file ibm_mfm.cpp
 * @brief HxC MFM track walk, IBM IDAM/DAM CRC, boot protection patterns.
 */
#include "dumpfloppy/ibm_mfm.hpp"

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
                std::vector<uint8_t>* boot_out)
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
            have_id = true;
            i = p;
            continue;
        }
        if ((mark == 0xFBu || mark == 0xF8u) && have_id)
        {
            const size_t nbytes = pending.bytes;
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

void fill_protection(flux_disk& d)
{
    d.protection.clear();
    unsigned long_n = 0;
    unsigned bad_dam = 0;
    unsigned bad_idam = 0;
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
        if (s.sector < 1u || s.sector > 9u)
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

} /* namespace */

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
        scan_track(file.subspan(toff, tsize), d.sectors, &d.boot);
    }
    fill_protection(d);
    add_boot_protection(d, d.boot);
    {
        uint32_t max_c = 0;
        uint32_t max_h = 0;
        uint32_t max_s = 0;
        for (const ibm_sector& s : d.sectors)
        {
            if (s.bytes == 512u && s.sector >= 1u && s.sector <= 18u &&
                s.data.size() >= 512u)
            {
                if (s.cyl > max_c)
                {
                    max_c = s.cyl;
                }
                if (s.head > max_h)
                {
                    max_h = s.head;
                }
                if (s.sector > max_s)
                {
                    max_s = s.sector;
                }
            }
        }
        if (max_s >= 8u)
        {
            const uint32_t cyls = max_c + 1u;
            const uint32_t heads = max_h + 1u;
            const uint32_t spt = max_s;
            d.assembled_chs.assign(static_cast<size_t>(cyls * heads * spt) * 512u, 0);
            for (const ibm_sector& s : d.sectors)
            {
                if (s.bytes != 512u || s.sector < 1u || s.sector > spt ||
                    s.data.size() < 512u)
                {
                    continue;
                }
                const size_t lba = (static_cast<size_t>(s.cyl) * heads + s.head) * spt +
                                   (s.sector - 1u);
                std::memcpy(d.assembled_chs.data() + lba * 512u, s.data.data(), 512u);
            }
        }
    }
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
