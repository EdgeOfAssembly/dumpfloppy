/**
 * @file sk_cga_palette.h
 * @brief CGA Palette
 * @see https://moddingwiki.shikadi.net/wiki/CGA_Palette
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CGA_PALETTE_H
#define DUMPFLOPPY_FORMATS_SK_CGA_PALETTE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_cga_palette final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CGA PALETTE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/CGA_Palette";
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
