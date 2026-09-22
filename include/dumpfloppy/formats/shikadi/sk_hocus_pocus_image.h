/**
 * @file sk_hocus_pocus_image.h
 * @brief Hocus Pocus Image Format
 * @see https://moddingwiki.shikadi.net/wiki/Hocus_Pocus_Image_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_HOCUS_POCUS_IMAGE_H
#define DUMPFLOPPY_FORMATS_SK_HOCUS_POCUS_IMAGE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_hocus_pocus_image final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "HOCUS IMAGE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Hocus_Pocus_Image_Format";
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
