/**
 * @file sk_raw_vga_image.h
 * @brief Raw VGA Image
 * @see https://moddingwiki.shikadi.net/wiki/Raw_VGA_Image
 */
#ifndef DUMPFLOPPY_FORMATS_SK_RAW_VGA_IMAGE_H
#define DUMPFLOPPY_FORMATS_SK_RAW_VGA_IMAGE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_raw_vga_image final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "RAW VGA";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Raw_VGA_Image";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
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
