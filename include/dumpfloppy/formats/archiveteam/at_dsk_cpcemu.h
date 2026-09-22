/**
 * @file at_dsk_cpcemu.h
 * @brief CPCEMU DSK image (Amstrad CPC / PCW / Spectrum +3).
 * @see http://fileformats.archiveteam.org/wiki/DSK_(CPCEMU)
 */
#ifndef DUMPFLOPPY_FORMATS_AT_DSK_CPCEMU_H
#define DUMPFLOPPY_FORMATS_AT_DSK_CPCEMU_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_dsk_cpcemu final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CPC DSK";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/DSK_(CPCEMU)";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        (void)data;
        return false;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
