/**
 * @file sk_skyroads_save_game.h
 * @brief SkyRoads save game format
 * @see https://moddingwiki.shikadi.net/wiki/SkyRoads_save_game_format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_SKYROADS_SAVE_GAME_H
#define DUMPFLOPPY_FORMATS_SK_SKYROADS_SAVE_GAME_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_skyroads_save_game final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SKYROADS GAME";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/SkyRoads_save_game_format";
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
