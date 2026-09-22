/**
 * @file sk_westwood_font_v4.h
 * @brief Westwood Font Format v4
 * @see https://moddingwiki.shikadi.net/wiki/Westwood_Font_Format_v4
 */
#ifndef DUMPFLOPPY_FORMATS_SK_WESTWOOD_FONT_V4_H
#define DUMPFLOPPY_FORMATS_SK_WESTWOOD_FONT_V4_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_westwood_font_v4 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "WW FONT V4";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Westwood_Font_Format_v4";
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
