/**
 * @file sk_kings_bounty_saved_game.h
 * @brief King's Bounty Saved game Format
 * @see https://moddingwiki.shikadi.net/wiki/King's_Bounty_Saved_game_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_KINGS_BOUNTY_SAVED_GAME_H
#define DUMPFLOPPY_FORMATS_SK_KINGS_BOUNTY_SAVED_GAME_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_kings_bounty_saved_game final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "KING GAME";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/King's_Bounty_Saved_game_Format";
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
