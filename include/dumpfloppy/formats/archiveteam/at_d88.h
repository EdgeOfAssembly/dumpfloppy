/**
 * @file at_d88.h
 * @brief D88 Japanese floppy disk image (.d88 / .1dd / .2dd).
 * @see http://fileformats.archiveteam.org/wiki/D88
 */
#ifndef DUMPFLOPPY_FORMATS_AT_D88_H
#define DUMPFLOPPY_FORMATS_AT_D88_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_d88 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "D88";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/D88";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* Wiki: little-endian dword at offset 20h is first-track offset
         * 0x02A0 or 0x02B0. */
        if (data.size() < 0x24u)
        {
            return false;
        }
        const uint32_t first_track = static_cast<uint32_t>(data[0x20])
            | (static_cast<uint32_t>(data[0x21]) << 8)
            | (static_cast<uint32_t>(data[0x22]) << 16)
            | (static_cast<uint32_t>(data[0x23]) << 24);
        return first_track == 0x02A0u || first_track == 0x02B0u;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
