/**
 * @file sk_solar_winds_sprites.h
 * @brief Solar Winds Sprites
 * @see https://moddingwiki.shikadi.net/wiki/Solar_Winds_Sprites
 */
#ifndef DUMPFLOPPY_FORMATS_SK_SOLAR_WINDS_SPRITES_H
#define DUMPFLOPPY_FORMATS_SK_SOLAR_WINDS_SPRITES_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_solar_winds_sprites final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SOLAR SPRITES";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Solar_Winds_Sprites";
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
