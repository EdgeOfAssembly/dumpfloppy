/**
 * @file at_imd.h
 * @brief Dave Dunfield ImageDisk (IMD) floppy image.
 * @see http://fileformats.archiveteam.org/wiki/IMD
 */
#ifndef DUMPFLOPPY_FORMATS_AT_IMD_H
#define DUMPFLOPPY_FORMATS_AT_IMD_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_imd final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "PC IMD";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/IMD";
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
