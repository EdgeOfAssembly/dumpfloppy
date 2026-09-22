/**
 * @file at_xdf.h
 * @brief IBM Extended Density Format floppy.
 * @see http://fileformats.archiveteam.org/wiki/XDF_(Extended_Density_Format)
 */
#ifndef DUMPFLOPPY_FORMATS_AT_XDF_H
#define DUMPFLOPPY_FORMATS_AT_XDF_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_xdf final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "PC XDF";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/XDF_(Extended_Density_Format)";
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
