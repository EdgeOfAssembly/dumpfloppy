/**
 * @file sk_duke_nukem_ii_palette.h
 * @brief Duke Nukem II Palette Formats
 * @see https://moddingwiki.shikadi.net/wiki/Duke_Nukem_II_Palette_Formats
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DUKE_NUKEM_II_PALETTE_H
#define DUMPFLOPPY_FORMATS_SK_DUKE_NUKEM_II_PALETTE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_duke_nukem_ii_palette final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DUKE PALETTE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Duke_Nukem_II_Palette_Formats";
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
