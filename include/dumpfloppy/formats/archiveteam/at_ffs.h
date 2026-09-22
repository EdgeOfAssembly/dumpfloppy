/**
 * @file at_ffs.h
 * @brief Amiga Fast File System (DOS\1 / DOS\3 / DOS\5).
 * @see http://fileformats.archiveteam.org/wiki/FFS
 */
#ifndef DUMPFLOPPY_FORMATS_AT_FFS_H
#define DUMPFLOPPY_FORMATS_AT_FFS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_ffs final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "AMIGA FFS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/FFS";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* Wiki: bootblock IDs DOS\1, DOS\3, DOS\5. */
        if (data.size() < 4)
        {
            return false;
        }
        return data[0] == static_cast<uint8_t>('D')
            && data[1] == static_cast<uint8_t>('O')
            && data[2] == static_cast<uint8_t>('S')
            && (data[3] == 0x01u || data[3] == 0x03u || data[3] == 0x05u);
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
