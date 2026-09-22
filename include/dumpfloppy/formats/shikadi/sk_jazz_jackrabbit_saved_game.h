/**
 * @file sk_jazz_jackrabbit_saved_game.h
 * @brief Jazz Jackrabbit Saved Game Format
 * @see https://moddingwiki.shikadi.net/wiki/Jazz_Jackrabbit_Saved_Game_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_JAZZ_JACKRABBIT_SAVED_GAME_H
#define DUMPFLOPPY_FORMATS_SK_JAZZ_JACKRABBIT_SAVED_GAME_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_jazz_jackrabbit_saved_game final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "JAZZ GAME";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Jazz_Jackrabbit_Saved_Game_Format";
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
