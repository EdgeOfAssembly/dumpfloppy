/**
 * @file sk_ultima_ii_dungeon.h
 * @brief Ultima II Dungeon Format
 * @see https://moddingwiki.shikadi.net/wiki/Ultima_II_Dungeon_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_ULTIMA_II_DUNGEON_H
#define DUMPFLOPPY_FORMATS_SK_ULTIMA_II_DUNGEON_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_ultima_ii_dungeon final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "ULTIMA DUNGEON";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Ultima_II_Dungeon_Format";
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
