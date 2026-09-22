/**
 * @file sk_stargunner_sprites.h
 * @brief Stargunner Sprites Format
 * @see https://moddingwiki.shikadi.net/wiki/Stargunner_Sprites_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_STARGUNNER_SPRITES_H
#define DUMPFLOPPY_FORMATS_SK_STARGUNNER_SPRITES_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_stargunner_sprites final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "STARGUNNER SPRIT";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Stargunner_Sprites_Format";
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
