/**
 * @file sk_dynamix_font_v6.h
 * @brief Dynamix Font Format v6
 * @see https://moddingwiki.shikadi.net/wiki/Dynamix_Font_Format_v6
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DYNAMIX_FONT_V6_H
#define DUMPFLOPPY_FORMATS_SK_DYNAMIX_FONT_V6_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dynamix_font_v6 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DYNAMIX V6";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Dynamix_Font_Format_v6";
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
