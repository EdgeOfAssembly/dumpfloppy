/**
 * @file sk_castles_ii_siege_and_conquest_save_game.h
 * @brief Castles II: Siege and Conquest Save Game Format
 * @see https://moddingwiki.shikadi.net/wiki/Castles_II:_Siege_and_Conquest_Save_Game_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CASTLES_II_SIEGE_AND_CONQUEST_SAVE_GAME_H
#define DUMPFLOPPY_FORMATS_SK_CASTLES_II_SIEGE_AND_CONQUEST_SAVE_GAME_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_castles_ii_siege_and_conquest_save_game final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CAST2 SAVE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Castles_II:_Siege_and_Conquest_Save_Game_Format";
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
