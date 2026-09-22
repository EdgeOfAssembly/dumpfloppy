/**
 * @file sk_4x4_off_road_racing_full_screen_graphic.h
 * @brief 4x4 Off-Road Racing Full Screen Graphic Format
 * @see https://moddingwiki.shikadi.net/wiki/4x4_Off-Road_Racing_Full_Screen_Graphic_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_4X4_OFF_ROAD_RACING_FULL_SCREEN_GRAPHIC_H
#define DUMPFLOPPY_FORMATS_SK_4X4_OFF_ROAD_RACING_FULL_SCREEN_GRAPHIC_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_4x4_off_road_racing_full_screen_graphic final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "4X4 FULL GFX";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/4x4_Off-Road_Racing_Full_Screen_Graphic_Format";
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
