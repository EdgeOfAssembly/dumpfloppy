/**
 * @file sk_kort_font.h
 * @brief KORT Font
 * @see https://moddingwiki.shikadi.net/wiki/KORT_Font
 */
#ifndef DUMPFLOPPY_FORMATS_SK_KORT_FONT_H
#define DUMPFLOPPY_FORMATS_SK_KORT_FONT_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_kort_font final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "KORT FONT";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/KORT_Font";
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
