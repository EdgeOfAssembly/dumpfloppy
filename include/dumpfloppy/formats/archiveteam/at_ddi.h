/**
 * @file at_ddi.h
 * @brief DiskDupe DDI floppy image.
 * @see http://fileformats.archiveteam.org/wiki/DDI
 */
#ifndef DUMPFLOPPY_FORMATS_AT_DDI_H
#define DUMPFLOPPY_FORMATS_AT_DDI_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_ddi final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DISKDUPE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/DDI";
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
