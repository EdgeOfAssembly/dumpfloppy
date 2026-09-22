/**
 * @file sk_dungeons_of_the_unforgiven_player_character.h
 * @brief Dungeons of the Unforgiven Player Character
 * @see https://moddingwiki.shikadi.net/wiki/Dungeons_of_the_Unforgiven_Player_Character
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DUNGEONS_OF_THE_UNFORGIVEN_PLAYER_CHARACTER_H
#define DUMPFLOPPY_FORMATS_SK_DUNGEONS_OF_THE_UNFORGIVEN_PLAYER_CHARACTER_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dungeons_of_the_unforgiven_player_character final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DOTU PLAYER";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Dungeons_of_the_Unforgiven_Player_Character";
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
