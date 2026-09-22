/**
 * @file sk_dune_2000_font.h
 * @brief Dune 2000 Font
 * @see https://moddingwiki.shikadi.net/wiki/Dune_2000_Font
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DUNE_2000_FONT_H
#define DUMPFLOPPY_FORMATS_SK_DUNE_2000_FONT_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dune_2000_font final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DUNE FONT";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Dune_2000_Font";
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
