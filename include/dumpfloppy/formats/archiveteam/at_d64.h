/**
 * @file at_d64.h
 * @brief Commodore D64 / D71 / D81 sector disk image.
 * @see http://fileformats.archiveteam.org/wiki/D64
 */
#ifndef DUMPFLOPPY_FORMATS_AT_D64_H
#define DUMPFLOPPY_FORMATS_AT_D64_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_d64 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "C64 D64";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/D64";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* Sizes from linked 1541/1571/1581 articles: 683, 1360, 3200 sectors
         * of 256 bytes; D64 page allows an optional 1-byte-per-sector error map. */
        const std::size_t n1541 = 683u * 256u;
        const std::size_t n1571 = 1360u * 256u;
        const std::size_t n1581 = 3200u * 256u;
        return data.size() == n1541 || data.size() == n1541 + 683u
            || data.size() == n1571 || data.size() == n1571 + 1360u
            || data.size() == n1581 || data.size() == n1581 + 3200u;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
