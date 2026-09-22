/**
 * @file sk_monster_bash_tileset.h
 * @brief Monster Bash Tileset Format
 * @see https://moddingwiki.shikadi.net/wiki/Monster_Bash_Tileset_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_MONSTER_BASH_TILESET_H
#define DUMPFLOPPY_FORMATS_SK_MONSTER_BASH_TILESET_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_monster_bash_tileset final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "MONSTER TILESET";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Monster_Bash_Tileset_Format";
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
