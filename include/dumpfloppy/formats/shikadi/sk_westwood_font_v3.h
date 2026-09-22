/**
 * @file sk_westwood_font_v3.h
 * @brief Westwood Font Format v3
 * @see https://moddingwiki.shikadi.net/wiki/Westwood_Font_Format_v3
 */
#ifndef DUMPFLOPPY_FORMATS_SK_WESTWOOD_FONT_V3_H
#define DUMPFLOPPY_FORMATS_SK_WESTWOOD_FONT_V3_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_westwood_font_v3 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "WW FONT V3";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Westwood_Font_Format_v3";
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
