/**
 * @file sk_4x4_off_road_racing_sprite.h
 * @brief 4x4 Off-Road Racing Sprite Format
 * @see https://moddingwiki.shikadi.net/wiki/4x4_Off-Road_Racing_Sprite_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_4X4_OFF_ROAD_RACING_SPRITE_H
#define DUMPFLOPPY_FORMATS_SK_4X4_OFF_ROAD_RACING_SPRITE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_4x4_off_road_racing_sprite final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "4X4 SPRITE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/4x4_Off-Road_Racing_Sprite_Format";
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
