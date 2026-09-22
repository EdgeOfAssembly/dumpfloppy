/**
 * @file sk_rescue_rover_saved_game.h
 * @brief Rescue Rover Saved game format
 * @see https://moddingwiki.shikadi.net/wiki/Rescue_Rover_Saved_game_format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_RESCUE_ROVER_SAVED_GAME_H
#define DUMPFLOPPY_FORMATS_SK_RESCUE_ROVER_SAVED_GAME_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_rescue_rover_saved_game final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "RESCUE GAME";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Rescue_Rover_Saved_game_format";
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
