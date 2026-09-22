/**
 * @file at_xpack.h
 * @brief XPACK compressed diskette image (.xdi).
 * @see http://fileformats.archiveteam.org/wiki/XPACK_disk_image
 */
#ifndef DUMPFLOPPY_FORMATS_AT_XPACK_H
#define DUMPFLOPPY_FORMATS_AT_XPACK_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_xpack final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "XPACK XDI";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/XPACK_disk_image";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* Wiki: starts with 'j' 'm' 0x02 0x04 and ends with 'j' 'm'. */
        if (data.size() < 6)
        {
            return false;
        }
        if (data[0] != static_cast<uint8_t>('j') || data[1] != static_cast<uint8_t>('m')
            || data[2] != 0x02u || data[3] != 0x04u)
        {
            return false;
        }
        return data[data.size() - 2u] == static_cast<uint8_t>('j')
            && data[data.size() - 1u] == static_cast<uint8_t>('m');
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
