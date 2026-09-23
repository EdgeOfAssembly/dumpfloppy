/**
 * @file at_woz.h
 * @brief Apple II WOZ disk image (Applesauce).
 * @see http://fileformats.archiveteam.org/wiki/WOZ_disk_image
 */
#ifndef DUMPFLOPPY_FORMATS_AT_WOZ_H
#define DUMPFLOPPY_FORMATS_AT_WOZ_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_woz final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "APPLE WOZ";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/WOZ_disk_image";
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
        const bool woz1 = data[0] == 'W' && data[1] == 'O' && data[2] == 'Z' &&
                          data[3] == '1';
        const bool woz2 = data[0] == 'W' && data[1] == 'O' && data[2] == 'Z' &&
                          data[3] == '2';
        if (!woz1 && !woz2)
        {
            return false;
        }
        return data[4] == 0xFFu && data[5] == 0x0Au && data[6] == 0x0Du &&
               data[7] == 0x0Au;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
