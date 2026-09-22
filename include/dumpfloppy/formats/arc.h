/**
 * @file arc.h
 * @brief SEA ARC (System Enhancement Associates) sequential archive (`.ARC`).
 *
 * Members are concatenated. Each starts with magic `0x1A`, then a 28-byte
 * header and packed payload. The archive ends at `0x1A 0x00` (method 0).
 *
 * Header (29 bytes, little-endian):
 *   1  magic 0x1A
 *   1  compression method (1–2 stored, 8 RLE+LZW, …; 0 = end)
 *  13  DOS 8.3 name, NUL-terminated
 *   4  packed size
 *   4  DOS date+time
 *   2  CRC-16 of uncompressed data
 *   4  uncompressed size
 *
 * @see http://fileformats.archiveteam.org/wiki/ARC_(compression_format)
 */
#ifndef DUMPFLOPPY_FORMATS_ARC_H
#define DUMPFLOPPY_FORMATS_ARC_H

#include "dumpfloppy/format.h"

#include <cstdint>
#include <cstring>

namespace dumpfloppy
{
namespace formats
{

#pragma pack(push, 1)
/**
 * @brief One SEA ARC member header (29 bytes).
 */
struct sea_arc_header
{
    uint8_t magic;           /**< Always 0x1A. */
    uint8_t method;          /**< 0 = end; 1–9 compression. */
    char name[13];           /**< 8.3, NUL-padded. */
    uint32_t packed_size;    /**< Payload bytes following this header. */
    uint16_t dos_date;
    uint16_t dos_time;
    uint16_t crc16;
    uint32_t unpacked_size;
};
#pragma pack(pop)

static_assert(sizeof(sea_arc_header) == 29, "SEA ARC header is 29 bytes");

/**
 * @brief SEA ARC (`.ARC`).
 */
class sea_arc final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SEA ARC";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/ARC_(compression_format)";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        if (data.size() < sizeof(sea_arc_header))
        {
            return false;
        }
        sea_arc_header h{};
        std::memcpy(&h, data.data(), sizeof(h));
        if (h.magic != 0x1Au)
        {
            return false;
        }
        if (h.method == 0u)
        {
            return data.size() >= 2u;
        }
        if (h.method > 9u)
        {
            return false;
        }
        bool name_ok = false;
        for (int i = 0; i < 13; ++i)
        {
            const unsigned char c = static_cast<unsigned char>(h.name[i]);
            if (c == 0u)
            {
                name_ok = (i > 0);
                break;
            }
            if (c < 0x20u || c > 0x7Eu)
            {
                return false;
            }
        }
        if (!name_ok)
        {
            return false;
        }
        const uint64_t need =
            static_cast<uint64_t>(sizeof(sea_arc_header)) + h.packed_size;
        return need <= data.size();
    }

    [[nodiscard]] bool match_name(std::string_view name) const override
    {
        return name_has_extension(name, "ARC");
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_FORMATS_ARC_H */
