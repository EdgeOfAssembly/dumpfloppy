/**
 * @file ipf_builder.hpp
 * @brief Encode a DD ADF as an SPS IPF with standard AmigaDOS tracks.
 */
#ifndef DUMPFLOPPY_TEST_IPF_BUILDER_HPP
#define DUMPFLOPPY_TEST_IPF_BUILDER_HPP

#include "dumpfloppy/amiga.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace dumpfloppy_test
{

inline uint32_t ipf_crc32r(const uint8_t* data, uint32_t size)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < size; ++i)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j)
        {
            if ((crc & 1u) != 0u)
            {
                crc = (crc >> 1) ^ 0xEDB88320u;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

inline void poke_be32(std::vector<uint8_t>& b, std::size_t off, uint32_t v)
{
    b[off] = static_cast<uint8_t>(v >> 24);
    b[off + 1u] = static_cast<uint8_t>(v >> 16);
    b[off + 2u] = static_cast<uint8_t>(v >> 8);
    b[off + 3u] = static_cast<uint8_t>(v);
}

inline uint32_t ipf_be32(const uint8_t* p)
{
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}

inline uint32_t amiga_csum(const uint32_t* addr, std::size_t nlong)
{
    uint32_t c = 0;
    for (std::size_t i = 0; i < nlong; ++i)
    {
        c ^= addr[i];
    }
    return ((c >> 1) & 0x55555555u) ^ (c & 0x55555555u);
}

inline void encode_mfm_long(uint32_t data, uint32_t& prev, std::vector<uint8_t>& cells)
{
    data &= 0x55555555u;
    const uint32_t data2 = data ^ 0x55555555u;
    data |= ((data2 >> 1) | 0x80000000u) & (data2 << 1);
    if ((prev & 1u) != 0u)
    {
        data &= 0x7FFFFFFFu;
    }
    prev = data;
    for (int i = 31; i >= 0; --i)
    {
        cells.push_back(static_cast<uint8_t>((data >> static_cast<unsigned>(i)) & 1u));
    }
}

inline void encode_block(const uint32_t* src, int nbytes, uint32_t& prev,
                         std::vector<uint8_t>& cells)
{
    const int nlong = nbytes / 4;
    for (int i = 0; i < nlong; ++i)
    {
        encode_mfm_long(src[i] >> 1, prev, cells);
    }
    for (int i = 0; i < nlong; ++i)
    {
        encode_mfm_long(src[i], prev, cells);
    }
}

inline std::vector<uint8_t> encode_amiga_sector(uint8_t track, uint8_t sector,
                                                const uint8_t* payload)
{
    uint32_t hdr[1];
    uint32_t labels[4] = {};
    uint32_t data[128];
    hdr[0] = (0xFFu << 24) | (static_cast<uint32_t>(track) << 16) |
             (static_cast<uint32_t>(sector) << 8) | (11u - sector);
    for (unsigned i = 0; i < 128u; ++i)
    {
        data[i] = ipf_be32(payload + i * 4u);
    }
    const uint32_t hchk = amiga_csum(hdr, 1) ^ amiga_csum(labels, 4);
    /* checksum() xors all header longs before hdrchk: magic+labels */
    uint32_t head_words[5] = {hdr[0], labels[0], labels[1], labels[2], labels[3]};
    const uint32_t header_chk = amiga_csum(head_words, 5);
    const uint32_t data_chk = amiga_csum(data, 128);
    (void)hchk;

    std::vector<uint8_t> cells;
    uint32_t prev = 0xAAAAAAAAu;
    /* leading 00 00 as AAAA then sync 4489 4489 */
    encode_mfm_long(0, prev, cells);
    for (int i = 15; i >= 0; --i)
    {
        cells.push_back(static_cast<uint8_t>((0x4489u >> static_cast<unsigned>(i)) & 1u));
    }
    for (int i = 15; i >= 0; --i)
    {
        cells.push_back(static_cast<uint8_t>((0x4489u >> static_cast<unsigned>(i)) & 1u));
    }
    prev = 0x4489u;
    encode_block(hdr, 4, prev, cells);
    encode_block(labels, 16, prev, cells);
    encode_block(&header_chk, 4, prev, cells);
    encode_block(&data_chk, 4, prev, cells);
    encode_block(data, 512, prev, cells);
    while ((cells.size() % 8u) != 0u)
    {
        cells.push_back(0);
    }
    std::vector<uint8_t> packed(cells.size() / 8u, 0);
    for (std::size_t i = 0; i < cells.size(); ++i)
    {
        if (cells[i] != 0u)
        {
            packed[i / 8u] =
                static_cast<uint8_t>(packed[i / 8u] | static_cast<uint8_t>(0x80u >> (i % 8u)));
        }
    }
    return packed;
}

inline void append_chunk(std::vector<uint8_t>& ipf, const char* tag,
                         const std::vector<uint8_t>& payload)
{
    const std::size_t t0 = ipf.size();
    ipf.insert(ipf.end(), tag, tag + 4);
    ipf.insert(ipf.end(), 8, 0);
    ipf.insert(ipf.end(), payload.begin(), payload.end());
    poke_be32(ipf, t0 + 4u, static_cast<uint32_t>(ipf.size() - t0));
    poke_be32(ipf, t0 + 8u, 0);
    const uint32_t crc =
        ipf_crc32r(ipf.data() + t0, static_cast<uint32_t>(ipf.size() - t0));
    poke_be32(ipf, t0 + 8u, crc);
}

inline std::vector<uint8_t> adf_to_ipf(const std::vector<uint8_t>& adf)
{
    if (adf.size() != dumpfloppy::k_adf_dd_bytes)
    {
        return {};
    }
    std::vector<uint8_t> ipf;
    ipf.insert(ipf.end(), {'C', 'A', 'P', 'S', 0, 0, 0, 12, 0, 0, 0, 0});
    poke_be32(ipf, 8, ipf_crc32r(ipf.data(), 12));

    std::vector<uint8_t> info(84, 0);
    poke_be32(info, 0, 1);  /* type floppy */
    poke_be32(info, 4, 1);  /* CAPS encoder */
    poke_be32(info, 8, 1);
    poke_be32(info, 12, 1); /* release */
    poke_be32(info, 24, 0);
    poke_be32(info, 28, 79);
    poke_be32(info, 32, 0);
    poke_be32(info, 36, 1);
    poke_be32(info, 48, 1); /* Amiga */
    append_chunk(ipf, "INFO", info);

    uint32_t idx = 1;
    for (uint8_t trk = 0; trk < 160u; ++trk)
    {
        bool used = (trk == 0u);
        const std::size_t base =
            static_cast<std::size_t>(trk) * 11u * dumpfloppy::k_adf_sector_bytes;
        for (std::size_t i = 0; i < 11u * dumpfloppy::k_adf_sector_bytes; ++i)
        {
            if (adf[base + i] != 0u)
            {
                used = true;
                break;
            }
        }
        if (!used)
        {
            continue;
        }

        std::vector<std::vector<uint8_t>> secs(11);
        uint32_t total_cells = 0;
        for (uint8_t s = 0; s < 11u; ++s)
        {
            secs[s] = encode_amiga_sector(
                trk, s, adf.data() + base + static_cast<std::size_t>(s) * 512u);
            total_cells += static_cast<uint32_t>(secs[s].size() * 8u);
        }

        std::vector<uint8_t> payload(32u * 11u, 0);
        for (uint8_t s = 0; s < 11u; ++s)
        {
            const uint32_t ncells = static_cast<uint32_t>(secs[s].size() * 8u);
            const uint32_t doff = static_cast<uint32_t>(payload.size());
            poke_be32(payload, static_cast<std::size_t>(s) * 32u, ncells);
            poke_be32(payload, static_cast<std::size_t>(s) * 32u + 4u, 0);
            poke_be32(payload, static_cast<std::size_t>(s) * 32u + 16u, 1);
            poke_be32(payload, static_cast<std::size_t>(s) * 32u + 28u, doff);
            /* type 1, 2-byte param of byte count */
            const uint32_t nbytes = static_cast<uint32_t>(secs[s].size());
            payload.push_back(0x41); /* nparam=2, type=1 */
            payload.push_back(static_cast<uint8_t>(nbytes >> 8));
            payload.push_back(static_cast<uint8_t>(nbytes));
            payload.insert(payload.end(), secs[s].begin(), secs[s].end());
            payload.push_back(0x00); /* end */
        }

        std::vector<uint8_t> imge(68, 0);
        poke_be32(imge, 0, static_cast<uint32_t>(trk / 2u));
        poke_be32(imge, 4, static_cast<uint32_t>(trk % 2u));
        poke_be32(imge, 8, 2);  /* type 2 standard */
        poke_be32(imge, 12, 1); /* 2us cells */
        poke_be32(imge, 28, total_cells);
        poke_be32(imge, 36, total_cells);
        poke_be32(imge, 40, 11);
        poke_be32(imge, 52, idx);
        append_chunk(ipf, "IMGE", imge);

        std::vector<uint8_t> dhdr(16, 0);
        poke_be32(dhdr, 0, static_cast<uint32_t>(payload.size()));
        poke_be32(dhdr, 4, total_cells);
        poke_be32(dhdr, 8, ipf_crc32r(payload.data(), static_cast<uint32_t>(payload.size())));
        poke_be32(dhdr, 12, idx);
        const std::size_t t0 = ipf.size();
        ipf.insert(ipf.end(), {'D', 'A', 'T', 'A', 0, 0, 0, 28, 0, 0, 0, 0});
        ipf.insert(ipf.end(), dhdr.begin(), dhdr.end());
        poke_be32(ipf, t0 + 8u, 0);
        poke_be32(ipf, t0 + 8u, ipf_crc32r(ipf.data() + t0, 28));
        ipf.insert(ipf.end(), payload.begin(), payload.end());
        ++idx;
    }
    return ipf;
}

} /* namespace dumpfloppy_test */

#endif /* DUMPFLOPPY_TEST_IPF_BUILDER_HPP */
