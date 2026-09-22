/**
 * @file sk_major_stryker_save_game.h
 * @brief Major Stryker Save Game Format
 * @see https://moddingwiki.shikadi.net/wiki/Major_Stryker_Save_Game_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_MAJOR_STRYKER_SAVE_GAME_H
#define DUMPFLOPPY_FORMATS_SK_MAJOR_STRYKER_SAVE_GAME_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_major_stryker_save_game final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "MAJOR GAME";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Major_Stryker_Save_Game_Format";
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
