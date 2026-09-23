/**
 * @file at_g64.h
 * @brief Commodore G64 GCR-coded raw disk image.
 * @see http://fileformats.archiveteam.org/wiki/G64
 */
#ifndef DUMPFLOPPY_FORMATS_AT_G64_H
#define DUMPFLOPPY_FORMATS_AT_G64_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_g64 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "C64 G64";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/G64";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        if (data.size() < 8u)
        {
            return false;
        }
        return data[0] == 'G' && data[1] == 'C' && data[2] == 'R' &&
               data[3] == '-' && data[4] == '1' && data[5] == '5' &&
               data[6] == '4' && data[7] == '1';
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
