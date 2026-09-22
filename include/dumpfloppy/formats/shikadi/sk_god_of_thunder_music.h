/**
 * @file sk_god_of_thunder_music.h
 * @brief God of Thunder Music Format
 * @see https://moddingwiki.shikadi.net/wiki/God_of_Thunder_Music_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_GOD_OF_THUNDER_MUSIC_H
#define DUMPFLOPPY_FORMATS_SK_GOD_OF_THUNDER_MUSIC_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_god_of_thunder_music final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "GOD MUSIC";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/God_of_Thunder_Music_Format";
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
