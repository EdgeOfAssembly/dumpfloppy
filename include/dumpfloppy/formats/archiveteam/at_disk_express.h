/**
 * @file at_disk_express.h
 * @brief Disk Express (.dxp) floppy image.
 * @see http://fileformats.archiveteam.org/wiki/Disk_Express
 */
#ifndef DUMPFLOPPY_FORMATS_AT_DISK_EXPRESS_H
#define DUMPFLOPPY_FORMATS_AT_DISK_EXPRESS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_disk_express final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DISK EXPRESS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/Disk_Express";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* Wiki: non-executable files begin with 'AS' then version 1 or 2. */
        if (data.size() < 3)
        {
            return false;
        }
        return data[0] == static_cast<uint8_t>('A')
            && data[1] == static_cast<uint8_t>('S')
            && (data[2] == 1u || data[2] == 2u);
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
