/**
 * @file sk_kris_music_system.h
 * @brief Kris' Music System Format
 * @see https://moddingwiki.shikadi.net/wiki/Kris'_Music_System_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_KRIS_MUSIC_SYSTEM_H
#define DUMPFLOPPY_FORMATS_SK_KRIS_MUSIC_SYSTEM_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_kris_music_system final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "KMS MUSIC";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Kris'_Music_System_Format";
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
