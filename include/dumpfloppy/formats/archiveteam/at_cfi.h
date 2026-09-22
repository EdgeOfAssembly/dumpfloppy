/**
 * @file at_cfi.h
 * @brief FDCOPY.COM compressed DOS floppy image (.cfi).
 * @see http://fileformats.archiveteam.org/wiki/CFI_disk_image
 */
#ifndef DUMPFLOPPY_FORMATS_AT_CFI_H
#define DUMPFLOPPY_FORMATS_AT_CFI_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_cfi final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "FDCOPY CFI";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/CFI_disk_image";
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
