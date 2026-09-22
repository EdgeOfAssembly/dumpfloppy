/**
 * @file at_raw_disk_image.h
 * @brief Headerless raw disk image (dd-style).
 * @see http://fileformats.archiveteam.org/wiki/Raw_disk_image
 */
#ifndef DUMPFLOPPY_FORMATS_AT_RAW_DISK_IMAGE_H
#define DUMPFLOPPY_FORMATS_AT_RAW_DISK_IMAGE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_raw_disk_image final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "RAW IMAGE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/Raw_disk_image";
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
