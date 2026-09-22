/**
 * @file sk_moraff_monster_map.h
 * @brief Moraff Monster Map
 * @see https://moddingwiki.shikadi.net/wiki/Moraff_Monster_Map
 */
#ifndef DUMPFLOPPY_FORMATS_SK_MORAFF_MONSTER_MAP_H
#define DUMPFLOPPY_FORMATS_SK_MORAFF_MONSTER_MAP_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_moraff_monster_map final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "MORAFF MAP";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Moraff_Monster_Map";
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
