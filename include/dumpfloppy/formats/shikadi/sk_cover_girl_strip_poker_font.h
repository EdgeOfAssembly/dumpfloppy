/**
 * @file sk_cover_girl_strip_poker_font.h
 * @brief Cover Girl Strip Poker Font
 * @see https://moddingwiki.shikadi.net/wiki/Cover_Girl_Strip_Poker_Font
 */
#ifndef DUMPFLOPPY_FORMATS_SK_COVER_GIRL_STRIP_POKER_FONT_H
#define DUMPFLOPPY_FORMATS_SK_COVER_GIRL_STRIP_POKER_FONT_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_cover_girl_strip_poker_font final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CGSP FONT";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Cover_Girl_Strip_Poker_Font";
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
