/**
 * @file sk_mystic_towers_pc_speaker_sound.h
 * @brief Mystic Towers PC Speaker Sound Format
 * @see https://moddingwiki.shikadi.net/wiki/Mystic_Towers_PC_Speaker_Sound_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_MYSTIC_TOWERS_PC_SPEAKER_SOUND_H
#define DUMPFLOPPY_FORMATS_SK_MYSTIC_TOWERS_PC_SPEAKER_SOUND_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_mystic_towers_pc_speaker_sound final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "MYSTIC SOUND";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Mystic_Towers_PC_Speaker_Sound_Format";
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
