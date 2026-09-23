/**
 * @file ipf.cpp
 * @brief SPS IPF track expand (CAPS DATA) and AmigaDOS sector assemble to ADF.
 *
 * Chunk layout and block descriptors follow MAME `ipf_dsk.cpp` (Olivier
 * Galibert). Amiga even/odd decode matches Linux `amiflop.c`.
 */
#include "dumpfloppy/foreign.hpp"
#include "dumpfloppy/amiga.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace dumpfloppy
{
namespace
{

uint32_t be32(std::span<const uint8_t> d, std::size_t off)
{
    return (static_cast<uint32_t>(d[off]) << 24) |
           (static_cast<uint32_t>(d[off + 1u]) << 16) |
           (static_cast<uint32_t>(d[off + 2u]) << 8) |
           static_cast<uint32_t>(d[off + 3u]);
}

uint32_t read_param(const uint8_t*& p, const uint8_t* end, unsigned nbytes)
{
    uint32_t v = 0;
    for (unsigned i = 0; i < nbytes; ++i)
    {
        if (p >= end)
        {
            return 0;
        }
        v = (v << 8) | *p++;
    }
    return v;
}

void write_raw_cells(std::vector<uint8_t>& cells, const uint8_t* data, uint32_t ncells,
                     bool& context)
{
    for (uint32_t i = 0; i < ncells; ++i)
    {
        const uint8_t bit =
            ((data[i >> 3] & static_cast<uint8_t>(0x80u >> (i & 7u))) != 0u) ? 1u : 0u;
        cells.push_back(bit);
        context = bit != 0u;
    }
}

void write_mfm_cells(std::vector<uint8_t>& cells, const uint8_t* data, uint32_t start,
                     uint32_t patlen, uint32_t ncells, bool& context)
{
    patlen *= 2u;
    for (uint32_t i = 0; i < ncells; ++i)
    {
        const uint32_t pos = (i + start) % patlen;
        const bool bit =
            (data[pos >> 4] & static_cast<uint8_t>(0x80u >> ((pos >> 1) & 7u))) != 0u;
        if ((pos & 1u) != 0u)
        {
            cells.push_back(bit ? 1u : 0u);
            context = bit;
        }
        else
        {
            cells.push_back((context || bit) ? 0u : 1u);
        }
    }
}

bool generate_block_data(const uint8_t* data, const uint8_t* dlimit, uint32_t ncells,
                         bool dmb, bool raw, bool& context, std::vector<uint8_t>& cells)
{
    const uint8_t* p = data;
    const std::size_t start = cells.size();
    for (;;)
    {
        if (p >= dlimit)
        {
            return false;
        }
        const uint8_t val = *p++;
        const unsigned nparam = static_cast<unsigned>(val >> 5);
        if (p + nparam > dlimit)
        {
            return false;
        }
        const uint32_t param = read_param(p, dlimit, nparam);
        const uint32_t typ = static_cast<uint32_t>(val & 0x1Fu);
        const uint64_t tleft = static_cast<uint64_t>(ncells) - (cells.size() - start);
        uint64_t bitcount = dmb ? param : static_cast<uint64_t>(param) * 8u;
        uint64_t bytecount = (bitcount + 7u) / 8u;
        switch (typ)
        {
        case 0:
            return tleft == 0u;
        case 1:
            if (raw || bitcount > tleft || bytecount > static_cast<uint64_t>(dlimit - p))
            {
                return false;
            }
            write_raw_cells(cells, p, static_cast<uint32_t>(bitcount), context);
            p += static_cast<std::size_t>(bytecount);
            break;
        case 2:
        case 3:
            if (raw || 2u * bitcount > tleft ||
                bytecount > static_cast<uint64_t>(dlimit - p))
            {
                return false;
            }
            write_mfm_cells(cells, p, 0, static_cast<uint32_t>(bitcount),
                            static_cast<uint32_t>(2u * bitcount), context);
            p += static_cast<std::size_t>(bytecount);
            break;
        case 4:
            if (!raw)
            {
                return false;
            }
            bitcount = static_cast<uint64_t>(param) * 8u;
            bytecount = param;
            if (bitcount > tleft || bytecount > static_cast<uint64_t>(dlimit - p))
            {
                return false;
            }
            write_raw_cells(cells, p, static_cast<uint32_t>(bitcount), context);
            p += static_cast<std::size_t>(bytecount);
            break;
        case 5:
            if (raw || 2u * bitcount > tleft)
            {
                return false;
            }
            cells.insert(cells.end(), static_cast<std::size_t>(2u * bitcount), 0u);
            context = false;
            break;
        default:
            return false;
        }
    }
}

uint16_t cells_u16(const std::vector<uint8_t>& cells, std::size_t bit)
{
    uint16_t v = 0;
    const std::size_t n = cells.size();
    for (unsigned k = 0; k < 16u; ++k)
    {
        v = static_cast<uint16_t>((v << 1) | cells[(bit + k) % n]);
    }
    return v;
}

uint32_t cells_u32(const std::vector<uint8_t>& cells, std::size_t bit)
{
    uint32_t v = 0;
    const std::size_t n = cells.size();
    for (unsigned k = 0; k < 32u; ++k)
    {
        v = (v << 1) | cells[(bit + k) % n];
    }
    return v;
}

uint32_t amiga_merge(uint32_t odd, uint32_t even)
{
    return ((odd & 0x55555555u) << 1) | (even & 0x55555555u);
}

uint32_t amiga_checksum(const uint32_t* addr, std::size_t nlong)
{
    uint32_t csum = 0;
    for (std::size_t i = 0; i < nlong; ++i)
    {
        csum ^= addr[i];
    }
    return ((csum >> 1) & 0x55555555u) ^ (csum & 0x55555555u);
}

bool decode_amiga_longs(const std::vector<uint8_t>& cells, std::size_t& bit,
                        uint32_t* out, std::size_t nlong)
{
    const std::size_t n = cells.size();
    if (n == 0u)
    {
        return false;
    }
    std::vector<uint32_t> odd(nlong);
    std::vector<uint32_t> even(nlong);
    for (std::size_t i = 0; i < nlong; ++i)
    {
        odd[i] = cells_u32(cells, bit);
        bit += 32u;
    }
    for (std::size_t i = 0; i < nlong; ++i)
    {
        even[i] = cells_u32(cells, bit);
        bit += 32u;
    }
    for (std::size_t i = 0; i < nlong; ++i)
    {
        out[i] = amiga_merge(odd[i], even[i]);
    }
    (void)n;
    return true;
}

void store_adf_sector(std::vector<uint8_t>& adf, uint8_t track, uint8_t sector,
                      const uint8_t* payload, uint32_t spt)
{
    if (sector >= spt || track >= 160u)
    {
        return;
    }
    const std::size_t off = (static_cast<std::size_t>(track) * spt + sector) *
                            k_adf_sector_bytes;
    if (off + k_adf_sector_bytes > adf.size())
    {
        return;
    }
    std::memcpy(adf.data() + off, payload, k_adf_sector_bytes);
}

void scan_amigados_track(const std::vector<uint8_t>& cells, std::vector<uint8_t>& adf,
                         uint32_t spt, uint32_t& found)
{
    if (cells.size() < 64u)
    {
        return;
    }
    const std::size_t n = cells.size();
    const std::size_t scan = n;
    std::size_t i = 0;
    while (i + 64u < scan)
    {
        if (cells_u16(cells, i) != 0x4489u)
        {
            ++i;
            continue;
        }
        std::size_t bit = i + 16u;
        while (cells_u16(cells, bit) == 0x4489u)
        {
            bit += 16u;
        }
        uint32_t hdr = 0;
        uint32_t labels[4] = {};
        uint32_t hchk = 0;
        uint32_t dchk = 0;
        uint32_t data[128] = {};
        if (!decode_amiga_longs(cells, bit, &hdr, 1) ||
            !decode_amiga_longs(cells, bit, labels, 4) ||
            !decode_amiga_longs(cells, bit, &hchk, 1) ||
            !decode_amiga_longs(cells, bit, &dchk, 1) ||
            !decode_amiga_longs(cells, bit, data, 128))
        {
            i += 16u;
            continue;
        }
        const uint8_t magic = static_cast<uint8_t>(hdr >> 24);
        const uint8_t trk = static_cast<uint8_t>(hdr >> 16);
        const uint8_t sec = static_cast<uint8_t>(hdr >> 8);
        uint32_t head_words[5] = {hdr, labels[0], labels[1], labels[2], labels[3]};
        if (magic != 0xFFu || amiga_checksum(head_words, 5) != hchk ||
            amiga_checksum(data, 128) != dchk)
        {
            i += 16u;
            continue;
        }
        uint8_t payload[512];
        for (unsigned w = 0; w < 128u; ++w)
        {
            payload[w * 4u] = static_cast<uint8_t>(data[w] >> 24);
            payload[w * 4u + 1u] = static_cast<uint8_t>(data[w] >> 16);
            payload[w * 4u + 2u] = static_cast<uint8_t>(data[w] >> 8);
            payload[w * 4u + 3u] = static_cast<uint8_t>(data[w]);
        }
        store_adf_sector(adf, trk, sec, payload, spt);
        ++found;
        i = bit;
    }
}

struct ipf_track
{
    uint32_t cyl = 0;
    uint32_t head = 0;
    uint32_t blocks = 0;
    uint32_t data_idx = 0;
    const uint8_t* data = nullptr;
    uint32_t data_size = 0;
    bool have_imge = false;
    bool have_data = false;
};

} /* namespace */

std::vector<uint8_t> assemble_ipf(std::span<const uint8_t> data)
{
    if (data.size() < 12u || std::memcmp(data.data(), "CAPS", 4) != 0 ||
        be32(data, 4) != 12u)
    {
        return {};
    }

    uint32_t encoder_type = 1;
    std::vector<ipf_track> tracks(200);
    auto take_track = [&](uint32_t idx) -> ipf_track* {
        if (idx >= 1000u)
        {
            return nullptr;
        }
        if (idx >= tracks.size())
        {
            tracks.resize(idx + 1u);
        }
        return &tracks[idx];
    };

    std::size_t off = 0;
    while (off + 12u <= data.size())
    {
        const uint32_t length = be32(data, off + 4u);
        if (length < 12u || off + length > data.size())
        {
            break;
        }
        const char* tag = reinterpret_cast<const char*>(data.data() + off);
        if (std::memcmp(tag, "INFO", 4) == 0 && length >= 96u)
        {
            encoder_type = be32(data, off + 16u);
        }
        else if (std::memcmp(tag, "IMGE", 4) == 0 && length >= 80u)
        {
            const uint32_t idx = be32(data, off + 64u);
            ipf_track* t = take_track(idx);
            if (t != nullptr)
            {
                t->cyl = be32(data, off + 12u);
                t->head = be32(data, off + 16u);
                t->blocks = be32(data, off + 52u);
                t->data_idx = idx;
                t->have_imge = true;
            }
        }
        else if (std::memcmp(tag, "DATA", 4) == 0 && length == 28u)
        {
            const uint32_t dsz = be32(data, off + 12u);
            const uint32_t idx = be32(data, off + 24u);
            if (off + 28u + dsz > data.size())
            {
                break;
            }
            ipf_track* t = take_track(idx);
            if (t != nullptr)
            {
                t->data = data.data() + off + 28u;
                t->data_size = dsz;
                t->have_data = true;
            }
            off += 28u + dsz;
            continue;
        }
        off += length;
    }

    std::vector<uint8_t> adf(k_adf_dd_bytes, 0);
    uint32_t spt = 11;
    uint32_t found = 0;
    for (ipf_track& t : tracks)
    {
        if (!t.have_imge || !t.have_data || t.blocks == 0u || t.data == nullptr)
        {
            continue;
        }
        if (t.data_size < 32u * t.blocks)
        {
            continue;
        }
        std::vector<uint8_t> cells;
        cells.reserve(120000);
        bool context = false;
        bool ok = true;
        const uint8_t* dend = t.data + t.data_size;
        for (uint32_t b = 0; b < t.blocks; ++b)
        {
            const uint8_t* th = t.data + 32u * b;
            uint32_t data_cells = (static_cast<uint32_t>(th[0]) << 24) |
                                  (static_cast<uint32_t>(th[1]) << 16) |
                                  (static_cast<uint32_t>(th[2]) << 8) |
                                  static_cast<uint32_t>(th[3]);
            uint32_t gap_cells = (static_cast<uint32_t>(th[4]) << 24) |
                                 (static_cast<uint32_t>(th[5]) << 16) |
                                 (static_cast<uint32_t>(th[6]) << 8) |
                                 static_cast<uint32_t>(th[7]);
            if (gap_cells < 8u)
            {
                gap_cells = 0;
            }
            const uint32_t enc = (static_cast<uint32_t>(th[16]) << 24) |
                                 (static_cast<uint32_t>(th[17]) << 16) |
                                 (static_cast<uint32_t>(th[18]) << 8) |
                                 static_cast<uint32_t>(th[19]);
            const uint32_t flags_raw = (static_cast<uint32_t>(th[20]) << 24) |
                                       (static_cast<uint32_t>(th[21]) << 16) |
                                       (static_cast<uint32_t>(th[22]) << 8) |
                                       static_cast<uint32_t>(th[23]);
            const uint32_t flags = (encoder_type == 1u) ? 0u : flags_raw;
            const uint32_t gap_type = flags & 3u;
            const bool dmb = (flags & 4u) != 0u;
            const bool raw = enc == 2u;
            if (enc != 1u && enc != 2u)
            {
                ok = false;
                break;
            }
            const uint32_t doff = (static_cast<uint32_t>(th[28]) << 24) |
                                  (static_cast<uint32_t>(th[29]) << 16) |
                                  (static_cast<uint32_t>(th[30]) << 8) |
                                  static_cast<uint32_t>(th[31]);
            if (doff >= t.data_size)
            {
                ok = false;
                break;
            }
            const std::size_t before = cells.size();
            if (!generate_block_data(t.data + doff, dend, data_cells, dmb, raw, context,
                                     cells) ||
                cells.size() - before != data_cells)
            {
                ok = false;
                break;
            }
            if (gap_type != 0u)
            {
                ok = false;
                break;
            }
            cells.insert(cells.end(), gap_cells, 0u);
        }
        if (!ok || cells.empty())
        {
            continue;
        }
        scan_amigados_track(cells, adf, spt, found);
    }
    if (found == 0u)
    {
        return {};
    }
    return adf;
}

} /* namespace dumpfloppy */
