/**
 * @file at_adf_amiga.h
 * @brief Amiga ADF raw disk image.
 * @see http://fileformats.archiveteam.org/wiki/ADF_(Amiga)
 */
#ifndef DUMPFLOPPY_FORMATS_AT_ADF_AMIGA_H
#define DUMPFLOPPY_FORMATS_AT_ADF_AMIGA_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_adf_amiga final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "AMIGA ADF";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/ADF_(Amiga)";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        if (data.size() < 4)
        {
            return false;
        }
        return data[0] == static_cast<uint8_t>('D')
            && data[1] == static_cast<uint8_t>('O')
            && data[2] == static_cast<uint8_t>('S')
            && data[3] <= 0x05u;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
