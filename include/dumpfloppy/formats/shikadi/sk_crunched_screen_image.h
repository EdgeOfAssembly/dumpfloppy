/**
 * @file sk_crunched_screen_image.h
 * @brief Crunched Screen Image
 * @see https://moddingwiki.shikadi.net/wiki/Crunched_Screen_Image
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CRUNCHED_SCREEN_IMAGE_H
#define DUMPFLOPPY_FORMATS_SK_CRUNCHED_SCREEN_IMAGE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_crunched_screen_image final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CRUNCHED IMAGE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Crunched_Screen_Image";
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
