/**
 * @file at_atr.h
 * @brief Atari 8-bit ATR (SIO2PC) disk image.
 * @see http://fileformats.archiveteam.org/wiki/ATR
 */
#ifndef DUMPFLOPPY_FORMATS_AT_ATR_H
#define DUMPFLOPPY_FORMATS_AT_ATR_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_atr final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "ATARI ATR";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/ATR";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* Wiki: little-endian identification word $9602. */
        if (data.size() < 2)
        {
            return false;
        }
        return data[0] == 0x02u && data[1] == 0x96u;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
