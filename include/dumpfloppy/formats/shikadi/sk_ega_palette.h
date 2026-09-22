/**
 * @file sk_ega_palette.h
 * @brief EGA Palette
 * @see https://moddingwiki.shikadi.net/wiki/EGA_Palette
 */
#ifndef DUMPFLOPPY_FORMATS_SK_EGA_PALETTE_H
#define DUMPFLOPPY_FORMATS_SK_EGA_PALETTE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_ega_palette final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "EGA PALETTE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/EGA_Palette";
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
