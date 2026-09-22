/**
 * @file sk_treasure_mountain_panning_image.h
 * @brief Treasure Mountain Panning Image Format
 * @see https://moddingwiki.shikadi.net/wiki/Treasure_Mountain_Panning_Image_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_TREASURE_MOUNTAIN_PANNING_IMAGE_H
#define DUMPFLOPPY_FORMATS_SK_TREASURE_MOUNTAIN_PANNING_IMAGE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_treasure_mountain_panning_image final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "TREASURE IMAGE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Treasure_Mountain_Panning_Image_Format";
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
