/**
 * @file sk_hocus_pocus_palette.h
 * @brief Hocus Pocus Palette Format
 * @see https://moddingwiki.shikadi.net/wiki/Hocus_Pocus_Palette_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_HOCUS_POCUS_PALETTE_H
#define DUMPFLOPPY_FORMATS_SK_HOCUS_POCUS_PALETTE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_hocus_pocus_palette final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "HOCUS PALETTE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Hocus_Pocus_Palette_Format";
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
