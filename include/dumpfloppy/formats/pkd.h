/**
 * @file pkd.h
 * @brief AGOS/Horrorsoft packed VGA blob (`.PKD`).
 *
 * Elvira 1 & 2 and Waxworks store one compressed graphics stream per
 * file (`NND.PKD`: zone + type 1 or 2). ScummVM AGOS loads these with
 * `decrunchFile` (Stuart Caie / PowerPacker-style, packed from the end).
 *
 * On-disk trailer (last 8 bytes, big-endian):
 *   uint32 bit-buffer seed
 *   uint32 uncompressed length
 *
 * @see https://github.com/scummvm/scummvm/blob/master/engines/agos/res.cpp
 * @see https://github.com/scummvm/scummvm-tools/blob/master/engines/agos/extract_agos.cpp
 */
#ifndef DUMPFLOPPY_FORMATS_PKD_H
#define DUMPFLOPPY_FORMATS_PKD_H

#include "dumpfloppy/format.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace dumpfloppy
{
namespace formats
{

#pragma pack(push, 1)
/**
 * @brief Last 8 bytes of a `.PKD` stream (big-endian fields).
 */
struct pkd_trailer
{
    uint32_t bitbuf_be;       /**< Bit-buffer seed (`size - 8`). */
    uint32_t dest_length_be;  /**< Uncompressed length (`size - 4`). */
};
#pragma pack(pop)

static_assert(sizeof(pkd_trailer) == 8, "PKD trailer is 8 bytes");

/** @brief Maximum uncompressed size accepted by the detector. */
inline constexpr uint32_t k_pkd_max_unpacked = 4u * 1024u * 1024u;

/**
 * @brief Read a big-endian uint32.
 */
[[nodiscard]] inline uint32_t pkd_be32(std::span<const uint8_t> data, size_t off)
{
    if (off + 4u > data.size())
    {
        return 0;
    }
    return (static_cast<uint32_t>(data[off]) << 24) |
           (static_cast<uint32_t>(data[off + 1u]) << 16) |
           (static_cast<uint32_t>(data[off + 2u]) << 8) |
           static_cast<uint32_t>(data[off + 3u]);
}

/**
 * @brief Uncompressed length from the trailer, or 0 if the file is too short.
 */
[[nodiscard]] inline uint32_t pkd_unpacked_size(std::span<const uint8_t> src)
{
    if (src.size() < 8u)
    {
        return 0;
    }
    return pkd_be32(src, src.size() - 4u);
}

/**
 * @brief Expand a PKD stream (ScummVM `AGOSEngine::decrunchFile`).
 *
 * @param[in]  src Packed bytes including the 8-byte trailer.
 * @param[out] dst Cleared and filled with @c destlen bytes on success.
 *
 * @retval true  @p dst holds the unpacked VGA blob.
 * @retval false Truncated input, overflow, or invalid bitstream.
 */
[[nodiscard]] inline bool pkd_decrunch(std::span<const uint8_t> src,
                                       std::vector<uint8_t>& dst)
{
    dst.clear();
    if (src.size() < 8u)
    {
        return false;
    }

    const uint32_t destlen = pkd_be32(src, src.size() - 4u);
    if (destlen == 0u || destlen > k_pkd_max_unpacked)
    {
        return false;
    }

    size_t s = src.size() - 4u;
    s -= 4u;
    uint32_t bb = pkd_be32(src, s);
    uint32_t x = bb;
    int bits = 0;
    while (x != 0u)
    {
        x >>= 1;
        ++bits;
    }
    --bits;

    dst.assign(destlen, 0);
    size_t d = destlen;

    auto getbit = [&]() -> uint32_t
    {
        if (bits == 0)
        {
            if (s < 4u)
            {
                return 0xFFFFFFFFu;
            }
            s -= 4u;
            bb = pkd_be32(src, s);
            bits = 31;
        }
        else
        {
            --bits;
        }
        const uint32_t bit = bb & 1u;
        bb >>= 1;
        return bit;
    };

    auto getbits = [&](unsigned nbits) -> uint32_t
    {
        uint32_t v = 0;
        for (unsigned i = 0; i < nbits; ++i)
        {
            const uint32_t bit = getbit();
            if (bit > 1u)
            {
                return 0xFFFFFFFFu;
            }
            v = (v << 1) | bit;
        }
        return v;
    };

    while (d > 0u)
    {
        uint32_t bit = getbit();
        if (bit > 1u)
        {
            dst.clear();
            return false;
        }

        unsigned type = 0;
        unsigned ncopy = 0;
        unsigned y = 0;
        if (bit != 0u)
        {
            x = getbits(2);
            if (x == 0xFFFFFFFFu)
            {
                dst.clear();
                return false;
            }
            switch (x)
            {
                case 0:
                    type = 1;
                    ncopy = 9;
                    y = 2;
                    break;
                case 1:
                    type = 1;
                    ncopy = 10;
                    y = 3;
                    break;
                case 2:
                    type = 1;
                    ncopy = 12;
                    y = getbits(8);
                    if (y == 0xFFFFFFFFu)
                    {
                        dst.clear();
                        return false;
                    }
                    break;
                default:
                    type = 0;
                    ncopy = 8;
                    y = 8;
                    break;
            }
        }
        else
        {
            x = getbit();
            if (x > 1u)
            {
                dst.clear();
                return false;
            }
            if (x != 0u)
            {
                type = 1;
                ncopy = 8;
                y = 1;
            }
            else
            {
                type = 0;
                ncopy = 3;
                y = 0;
            }
        }

        if (type == 0u)
        {
            x = getbits(ncopy);
            if (x == 0xFFFFFFFFu)
            {
                dst.clear();
                return false;
            }
            y += x;
            if (y + 1u > d)
            {
                dst.clear();
                return false;
            }
            do
            {
                x = getbits(8);
                if (x == 0xFFFFFFFFu)
                {
                    dst.clear();
                    return false;
                }
                --d;
                dst[d] = static_cast<uint8_t>(x);
            } while (y-- > 0u);
        }
        else
        {
            if (y + 1u > d)
            {
                dst.clear();
                return false;
            }
            x = getbits(ncopy);
            if (x == 0xFFFFFFFFu)
            {
                dst.clear();
                return false;
            }
            if (d + x > destlen)
            {
                dst.clear();
                return false;
            }
            do
            {
                --d;
                dst[d] = dst[d + x];
            } while (y-- > 0u);
        }
    }

    return true;
}

/**
 * @brief AGOS packed `.PKD` (Elvira / Waxworks graphics).
 */
class pkd final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "AGOS PKD";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://github.com/scummvm/scummvm/blob/master/engines/agos/res.cpp";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        const uint32_t destlen = pkd_unpacked_size(data);
        if (destlen == 0u || destlen < data.size() || destlen > k_pkd_max_unpacked)
        {
            return false;
        }
        std::vector<uint8_t> out;
        return pkd_decrunch(data, out) && out.size() == destlen;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_FORMATS_PKD_H */
