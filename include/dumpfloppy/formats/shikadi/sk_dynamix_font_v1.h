/**
 * @file sk_dynamix_font_v1.h
 * @brief Dynamix Font Format v1
 * @see https://moddingwiki.shikadi.net/wiki/Dynamix_Font_Format_v1
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DYNAMIX_FONT_V1_H
#define DUMPFLOPPY_FORMATS_SK_DYNAMIX_FONT_V1_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dynamix_font_v1 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DYNAMIX V1";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Dynamix_Font_Format_v1";
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
