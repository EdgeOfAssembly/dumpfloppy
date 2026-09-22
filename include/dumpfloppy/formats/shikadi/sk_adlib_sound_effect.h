/**
 * @file sk_adlib_sound_effect.h
 * @brief Adlib sound effect
 * @see https://moddingwiki.shikadi.net/wiki/Adlib_sound_effect
 */
#ifndef DUMPFLOPPY_FORMATS_SK_ADLIB_SOUND_EFFECT_H
#define DUMPFLOPPY_FORMATS_SK_ADLIB_SOUND_EFFECT_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_adlib_sound_effect final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "ADLIB SFX";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Adlib_sound_effect";
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
