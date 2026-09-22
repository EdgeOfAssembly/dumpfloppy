/**
 * @file at_ofs.h
 * @brief Amiga Old File System (DOS\0 / DOS\2 / DOS\4).
 * @see http://fileformats.archiveteam.org/wiki/OFS
 */
#ifndef DUMPFLOPPY_FORMATS_AT_OFS_H
#define DUMPFLOPPY_FORMATS_AT_OFS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_ofs final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "AMIGA OFS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/OFS";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* FFS article documents OFS bootblock IDs DOS\0, DOS\2, DOS\4. */
        if (data.size() < 4)
        {
            return false;
        }
        return data[0] == static_cast<uint8_t>('D')
            && data[1] == static_cast<uint8_t>('O')
            && data[2] == static_cast<uint8_t>('S')
            && (data[3] == 0x00u || data[3] == 0x02u || data[3] == 0x04u);
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
