/**
 * @file sk_space_crusade_savegame.h
 * @brief Space Crusade Savegame Format
 * @see https://moddingwiki.shikadi.net/wiki/Space_Crusade_Savegame_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_SPACE_CRUSADE_SAVEGAME_H
#define DUMPFLOPPY_FORMATS_SK_SPACE_CRUSADE_SAVEGAME_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_space_crusade_savegame final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SPACE SAVEGAME";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Space_Crusade_Savegame_Format";
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
