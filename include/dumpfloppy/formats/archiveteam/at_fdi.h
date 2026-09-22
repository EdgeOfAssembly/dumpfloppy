/**
 * @file at_fdi.h
 * @brief Disk2FDI Formatted Disk Image.
 * @see http://fileformats.archiveteam.org/wiki/FDI
 */
#ifndef DUMPFLOPPY_FORMATS_AT_FDI_H
#define DUMPFLOPPY_FORMATS_AT_FDI_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_fdi final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "PC FDI";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/FDI";
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
