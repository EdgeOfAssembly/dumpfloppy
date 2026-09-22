/**
 * @file sk_word_rescue_saved_game.h
 * @brief Word Rescue Saved Game Format
 * @see https://moddingwiki.shikadi.net/wiki/Word_Rescue_Saved_Game_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_WORD_RESCUE_SAVED_GAME_H
#define DUMPFLOPPY_FORMATS_SK_WORD_RESCUE_SAVED_GAME_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_word_rescue_saved_game final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "WORD GAME";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Word_Rescue_Saved_Game_Format";
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
