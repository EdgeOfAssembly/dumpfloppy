/**
 * @file at_fat.h
 * @brief FAT filesystem family (see also fat12.h for FAT12 volumes).
 * @see http://fileformats.archiveteam.org/wiki/FAT
 */
#ifndef DUMPFLOPPY_FORMATS_AT_FAT_H
#define DUMPFLOPPY_FORMATS_AT_FAT_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_fat final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "FAT";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/FAT";
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
