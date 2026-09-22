/**
 * @file sk_microsoft_vector_font.h
 * @brief Microsoft Vector Font Format
 * @see https://moddingwiki.shikadi.net/wiki/Microsoft_Vector_Font_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_MICROSOFT_VECTOR_FONT_H
#define DUMPFLOPPY_FORMATS_SK_MICROSOFT_VECTOR_FONT_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_microsoft_vector_font final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "MS VEC FONT";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Microsoft_Vector_Font_Format";
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
