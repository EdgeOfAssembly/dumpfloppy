/**
 * @file sk_dynamix_font_v4_v5.h
 * @brief Dynamix Font Format v4-v5
 * @see https://moddingwiki.shikadi.net/wiki/Dynamix_Font_Format_v4-v5
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DYNAMIX_FONT_V4_V5_H
#define DUMPFLOPPY_FORMATS_SK_DYNAMIX_FONT_V4_V5_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dynamix_font_v4_v5 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DYNAMIX V4-V5";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Dynamix_Font_Format_v4-v5";
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
