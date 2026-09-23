/**
 * @file at_g71.h
 * @brief Commodore G71 GCR-coded 1571 disk image (VICE GCR-1571).
 * @see https://vice-emu.sourceforge.io/vice_17.html
 */
#ifndef DUMPFLOPPY_FORMATS_AT_G71_H
#define DUMPFLOPPY_FORMATS_AT_G71_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_g71 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "C64 G71";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://vice-emu.sourceforge.io/vice_17.html";
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
               data[6] == '7' && data[7] == '1';
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
