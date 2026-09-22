/**
 * @file sk_battle_isle_history_line_map.h
 * @brief Battle Isle / History Line map format
 * @see https://moddingwiki.shikadi.net/wiki/Battle_Isle_/_History_Line_map_format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_BATTLE_ISLE_HISTORY_LINE_MAP_H
#define DUMPFLOPPY_FORMATS_SK_BATTLE_ISLE_HISTORY_LINE_MAP_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_battle_isle_history_line_map final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "BI/HL MAP";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Battle_Isle_/_History_Line_map_format";
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
