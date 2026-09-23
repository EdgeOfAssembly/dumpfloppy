/**
 * @file foreign.cpp
 * @brief Identify SPS IPF, Apple WOZ, Pasti STX, and Apple 2IMG containers.
 */
#include "dumpfloppy/foreign.hpp"
#include "dumpfloppy/image.hpp"

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

uint32_t le32(std::span<const uint8_t> d, std::size_t off)
{
    return static_cast<uint32_t>(d[off]) |
           (static_cast<uint32_t>(d[off + 1u]) << 8) |
           (static_cast<uint32_t>(d[off + 2u]) << 16) |
           (static_cast<uint32_t>(d[off + 3u]) << 24);
}

uint32_t be32(std::span<const uint8_t> d, std::size_t off)
{
    return (static_cast<uint32_t>(d[off]) << 24) |
           (static_cast<uint32_t>(d[off + 1u]) << 16) |
           (static_cast<uint32_t>(d[off + 2u]) << 8) |
           static_cast<uint32_t>(d[off + 3u]);
}

bool magic4(std::span<const uint8_t> d, const char* m)
{
    return d.size() >= 4u && std::memcmp(d.data(), m, 4) == 0;
}

const char* ipf_platform_name(uint32_t id)
{
    switch (id)
    {
    case 1:
        return "Amiga";
    case 2:
        return "Atari ST";
    case 3:
        return "IBM PC";
    case 4:
        return "Amstrad CPC";
    case 5:
        return "ZX Spectrum";
    case 6:
        return "Sam Coupe";
    case 7:
        return "Archimedes";
    case 8:
        return "Commodore 64";
    case 9:
        return "Atari 8-bit";
    default:
        return "unknown";
    }
}

foreign_disk parse_ipf(std::span<const uint8_t> data)
{
    foreign_disk out{};
    if (data.size() < 12u || !magic4(data, "CAPS"))
    {
        return out;
    }
    if (be32(data, 4) != 12u)
    {
        return out;
    }

    std::size_t off = 0;
    uint32_t imge = 0;
    while (off + 12u <= data.size())
    {
        const uint32_t length = be32(data, off + 4u);
        if (length < 12u || off + length > data.size())
        {
            break;
        }
        const char* name = reinterpret_cast<const char*>(data.data() + off);
        if (std::memcmp(name, "INFO", 4) == 0 && length >= 12u + 68u)
        {
            const std::size_t p = off + 12u;
            out.file_id = be32(data, p + 12u);
            out.min_cylinder = be32(data, p + 24u);
            out.max_cylinder = be32(data, p + 28u);
            out.min_head = be32(data, p + 32u);
            out.max_head = be32(data, p + 36u);
            out.platform = ipf_platform_name(be32(data, p + 48u));
            out.note = "SPS CAPS flux; filesystem not decoded in this version";
        }
        else if (std::memcmp(name, "IMGE", 4) == 0)
        {
            ++imge;
        }
        off += length;
    }
    out.present = true;
    out.kind = foreign_kind::ipf;
    out.format = "SPS IPF";
    out.track_count = imge;
    if (out.platform.empty())
    {
        out.platform = "unknown";
    }
    return out;
}

foreign_disk parse_woz(std::span<const uint8_t> data)
{
    foreign_disk out{};
    if (data.size() < 16u)
    {
        return out;
    }
    const bool woz1 = std::memcmp(data.data(), "WOZ1", 4) == 0;
    const bool woz2 = std::memcmp(data.data(), "WOZ2", 4) == 0;
    if (!woz1 && !woz2)
    {
        return out;
    }
    static constexpr uint8_t k_lf[4] = {0xFFu, 0x0Au, 0x0Du, 0x0Au};
    if (std::memcmp(data.data() + 4, k_lf, 4) != 0)
    {
        return out;
    }

    out.present = true;
    out.kind = foreign_kind::woz;
    out.format = woz2 ? "APPLE WOZ2" : "APPLE WOZ1";
    out.platform = "Apple II";
    out.note = "Applesauce flux; DOS 3.3/ProDOS not decoded in this version";

    std::size_t off = 12;
    while (off + 8u <= data.size())
    {
        const uint32_t size = le32(data, off + 4u);
        if (off + 8u + size > data.size())
        {
            break;
        }
        if (std::memcmp(data.data() + off, "INFO", 4) == 0 && size >= 36u)
        {
            const std::size_t p = off + 8u;
            const uint8_t disk_type = data[p + 1u];
            out.write_protected = data[p + 2u] != 0u;
            char creator[33]{};
            std::memcpy(creator, data.data() + p + 5u, 32u);
            out.creator = creator;
            while (!out.creator.empty() && out.creator.back() == ' ')
            {
                out.creator.pop_back();
            }
            if (disk_type == 1u)
            {
                out.note = "5.25-inch Apple II WOZ; GCR not decoded in this version";
            }
            else if (disk_type == 2u)
            {
                out.note = "3.5-inch Apple II WOZ; GCR not decoded in this version";
            }
        }
        else if (std::memcmp(data.data() + off, "TMAP", 4) == 0)
        {
            uint32_t used = 0;
            for (std::size_t i = 0; i < size; ++i)
            {
                if (data[off + 8u + i] != 0xFFu)
                {
                    ++used;
                }
            }
            out.track_count = used;
        }
        off += 8u + size;
    }
    return out;
}

foreign_disk parse_stx(std::span<const uint8_t> data)
{
    foreign_disk out{};
    if (data.size() < 16u || data[0] != 'R' || data[1] != 'S' || data[2] != 'Y' ||
        data[3] != 0)
    {
        return out;
    }
    out.present = true;
    out.kind = foreign_kind::stx;
    out.format = "ATARI STX";
    out.platform = "Atari ST";
    out.track_count = data[10];
    {
        const unsigned ver = le16(data, 4);
        out.note = "Pasti STX v" + std::to_string(ver >> 8) + "." +
                   std::to_string(ver & 0xFFu) +
                   " (copy-protected); GEMDOS not decoded in this version";
    }
    return out;
}

foreign_disk parse_2img(std::span<const uint8_t> data)
{
    foreign_disk out{};
    if (data.size() < 30u || !magic4(data, "2IMG"))
    {
        return out;
    }
    out.present = true;
    out.kind = foreign_kind::img2mg;
    out.format = "APPLE 2IMG";
    out.platform = "Apple II";
    char creator[5]{};
    std::memcpy(creator, data.data() + 4, 4u);
    out.creator = creator;
    const uint16_t fmt = le16(data, 12);
    out.data_offset = le32(data, 22);
    out.data_length = le32(data, 26);
    if (fmt == 0u)
    {
        out.note = "2IMG DOS 3.3 payload; catalog not decoded in this version";
    }
    else if (fmt == 1u)
    {
        out.note = "2IMG ProDOS payload; catalog not decoded in this version";
    }
    else if (fmt == 2u)
    {
        out.note = "2IMG nibble payload; GCR not decoded in this version";
    }
    else
    {
        out.note = "2IMG prefix; payload not decoded in this version";
    }
    return out;
}

} /* namespace */

foreign_disk parse_foreign(std::span<const uint8_t> data)
{
    foreign_disk d = parse_ipf(data);
    if (d.present)
    {
        return d;
    }
    d = parse_woz(data);
    if (d.present)
    {
        return d;
    }
    d = parse_stx(data);
    if (d.present)
    {
        return d;
    }
    return parse_2img(data);
}

flux_disk assemble_stx(std::span<const uint8_t> data)
{
    flux_disk flux{};
    if (data.size() < 16u || data[0] != 'R' || data[1] != 'S' || data[2] != 'Y' ||
        data[3] != 0)
    {
        return flux;
    }

    constexpr uint16_t k_flag_sectors = 0x0001u;
    constexpr uint16_t k_flag_image = 0x0040u;
    constexpr uint16_t k_flag_sync = 0x0080u;
    constexpr uint8_t k_fdc_crc = 0x08u;
    constexpr uint8_t k_fdc_rnf = 0x10u;

    const uint8_t ntracks = data[10];
    flux.present = true;
    flux.format_name = "ATARI STX";
    flux.tracks = ntracks;

    struct placed
    {
        uint8_t cyl;
        uint8_t head;
        uint8_t sector;
        uint8_t size_code;
        uint16_t bytes;
        bool crc_ok;
        bool rnf;
        std::vector<uint8_t> payload;
    };
    std::vector<placed> found;

    std::size_t p = 16;
    for (uint8_t t = 0; t < ntracks && p + 16u <= data.size(); ++t)
    {
        const uint32_t rec = le32(data, p);
        const uint32_t fuzzy = le32(data, p + 4u);
        const uint16_t nsec = le16(data, p + 8u);
        const uint16_t flags = le16(data, p + 10u);
        const std::size_t pnext = p + rec;
        if (rec < 16u || pnext > data.size())
        {
            break;
        }
        p += 16u;

        struct sdesc
        {
            uint32_t off;
            uint8_t id_c;
            uint8_t id_h;
            uint8_t id_s;
            uint8_t id_n;
            uint8_t fdc;
        };
        std::vector<sdesc> descs;
        if ((flags & k_flag_sectors) != 0u)
        {
            for (uint16_t s = 0; s < nsec; ++s)
            {
                if (p + 16u > data.size())
                {
                    break;
                }
                sdesc d{};
                d.off = le32(data, p);
                d.id_c = data[p + 8u];
                d.id_h = data[p + 9u];
                d.id_s = data[p + 10u];
                d.id_n = data[p + 11u];
                d.fdc = data[p + 14u];
                descs.push_back(d);
                p += 16u;
            }
        }
        if (fuzzy > 0u)
        {
            if (p + fuzzy > data.size())
            {
                break;
            }
            flux.protection.emplace_back("fuzzy mask " + std::to_string(fuzzy) +
                                         " bytes on a track");
            p += fuzzy;
        }

        const std::size_t data_base = p;
        if ((flags & k_flag_image) != 0u)
        {
            std::size_t q = p;
            if ((flags & k_flag_sync) != 0u)
            {
                q += 2u;
            }
            if (q + 2u > data.size())
            {
                break;
            }
            const uint16_t img_sz = le16(data, q);
            q += 2u;
            if (q + img_sz > data.size())
            {
                break;
            }
        }

        for (const sdesc& d : descs)
        {
            const uint16_t nbytes = static_cast<uint16_t>(
                128u << (static_cast<unsigned>(d.id_n) & 3u));
            placed sec{};
            sec.cyl = d.id_c;
            sec.head = d.id_h;
            sec.sector = d.id_s;
            sec.size_code = static_cast<uint8_t>(d.id_n & 3u);
            sec.bytes = nbytes;
            sec.crc_ok = (d.fdc & k_fdc_crc) == 0u;
            sec.rnf = (d.fdc & k_fdc_rnf) != 0u;
            const std::size_t off = data_base + d.off;
            if (!sec.rnf && off + nbytes <= data.size() && off >= data_base)
            {
                sec.payload.assign(data.begin() + static_cast<std::ptrdiff_t>(off),
                                   data.begin() + static_cast<std::ptrdiff_t>(off + nbytes));
            }
            if (sec.size_code != 2u)
            {
                flux.protection.push_back(
                    "nonstandard sector C" + std::to_string(sec.cyl) + ":H" +
                    std::to_string(sec.head) + ":S" + std::to_string(sec.sector) +
                    " " + std::to_string(sec.bytes) + " bytes");
            }
            if (!sec.crc_ok)
            {
                flux.protection.push_back("CRC error C" + std::to_string(sec.cyl) +
                                          ":H" + std::to_string(sec.head) + ":S" +
                                          std::to_string(sec.sector));
            }
            found.push_back(std::move(sec));
        }
        p = pnext;
    }

    uint32_t max_c = 0;
    uint32_t max_h = 0;
    uint32_t max_s = 0;
    for (const placed& s : found)
    {
        ibm_sector is{};
        is.cyl = s.cyl;
        is.head = s.head;
        is.sector = s.sector;
        is.size_code = s.size_code;
        is.bytes = s.bytes;
        is.idam_crc_ok = true;
        is.dam_crc_ok = s.crc_ok;
        is.has_dam = !s.payload.empty();
        is.data = s.payload;
        flux.sectors.push_back(std::move(is));
        if (s.size_code == 2u && s.sector >= 1u && !s.rnf && s.payload.size() == 512u)
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

    if (max_s == 0u || max_s > 18u || max_h > 1u || max_c > 83u)
    {
        if (max_s == 0u)
        {
            flux.note = "no standard 512-byte sectors to assemble";
        }
        return flux;
    }

    const uint32_t cyls = max_c + 1u;
    const uint32_t heads = max_h + 1u;
    const uint32_t spt = max_s;
    const std::size_t img_bytes =
        static_cast<std::size_t>(cyls) * heads * spt * 512u;
    if (img_bytes == 0u || img_bytes > k_max_image_bytes)
    {
        return flux;
    }
    flux.assembled_chs.assign(img_bytes, 0);
    flux.chs_cyls = cyls;
    flux.chs_heads = heads;
    flux.chs_spt = spt;
    flux.sides = heads;
    flux.note = "standard 512-byte sectors assembled for GEMDOS/FAT";

    for (const placed& s : found)
    {
        if (s.size_code != 2u || s.sector < 1u || s.rnf || s.payload.size() != 512u)
        {
            continue;
        }
        if (s.cyl >= cyls || s.head >= heads || s.sector > spt)
        {
            continue;
        }
        const std::size_t lba = (static_cast<std::size_t>(s.cyl) * heads + s.head) * spt +
                                (static_cast<std::size_t>(s.sector) - 1u);
        std::memcpy(flux.assembled_chs.data() + lba * 512u, s.payload.data(), 512u);
        if (s.cyl == 0u && s.head == 0u && s.sector == 1u)
        {
            flux.boot = s.payload;
        }
    }
    return flux;
}

} /* namespace dumpfloppy */
