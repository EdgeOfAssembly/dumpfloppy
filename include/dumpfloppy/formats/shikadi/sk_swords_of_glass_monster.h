/**
 * @file sk_swords_of_glass_monster.h
 * @brief Swords of Glass monster format
 * @see https://moddingwiki.shikadi.net/wiki/Swords_of_Glass_monster_format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_SWORDS_OF_GLASS_MONSTER_H
#define DUMPFLOPPY_FORMATS_SK_SWORDS_OF_GLASS_MONSTER_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_swords_of_glass_monster final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SWORDS MONSTER";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Swords_of_Glass_monster_format";
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
