/**
 * @file sk_hocus_pocus_sprite.h
 * @brief Hocus Pocus Sprite Format
 * @see https://moddingwiki.shikadi.net/wiki/Hocus_Pocus_Sprite_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_HOCUS_POCUS_SPRITE_H
#define DUMPFLOPPY_FORMATS_SK_HOCUS_POCUS_SPRITE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_hocus_pocus_sprite final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "HOCUS SPRITE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Hocus_Pocus_Sprite_Format";
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
