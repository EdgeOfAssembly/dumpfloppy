/**
 * @file at_a2r.h
 * @brief Applesauce A2R flux disk image.
 * @see http://fileformats.archiveteam.org/wiki/A2R_disk_image
 */
#ifndef DUMPFLOPPY_FORMATS_AT_A2R_H
#define DUMPFLOPPY_FORMATS_AT_A2R_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_a2r final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "APPLE A2R";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/A2R_disk_image";
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
