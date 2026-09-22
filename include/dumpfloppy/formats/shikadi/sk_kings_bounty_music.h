/**
 * @file sk_kings_bounty_music.h
 * @brief King's Bounty Music Format
 * @see https://moddingwiki.shikadi.net/wiki/King's_Bounty_Music_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_KINGS_BOUNTY_MUSIC_H
#define DUMPFLOPPY_FORMATS_SK_KINGS_BOUNTY_MUSIC_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_kings_bounty_music final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "KING MUSIC";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/King's_Bounty_Music_Format";
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
