/**
 * @file sk_the_blues_brothers_sprite.h
 * @brief The Blues Brothers Sprite Format
 * @see https://moddingwiki.shikadi.net/wiki/The_Blues_Brothers_Sprite_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_THE_BLUES_BROTHERS_SPRITE_H
#define DUMPFLOPPY_FORMATS_SK_THE_BLUES_BROTHERS_SPRITE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_the_blues_brothers_sprite final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "BLUES SPRITE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/The_Blues_Brothers_Sprite_Format";
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
