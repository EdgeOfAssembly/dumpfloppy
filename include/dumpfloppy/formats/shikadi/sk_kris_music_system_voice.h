/**
 * @file sk_kris_music_system_voice.h
 * @brief Kris' Music System Voice Format
 * @see https://moddingwiki.shikadi.net/wiki/Kris'_Music_System_Voice_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_KRIS_MUSIC_SYSTEM_VOICE_H
#define DUMPFLOPPY_FORMATS_SK_KRIS_MUSIC_SYSTEM_VOICE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_kris_music_system_voice final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "KMS VOICE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Kris'_Music_System_Voice_Format";
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
