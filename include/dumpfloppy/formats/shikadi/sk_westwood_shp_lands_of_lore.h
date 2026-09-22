/**
 * @file sk_westwood_shp_lands_of_lore.h
 * @brief Westwood SHP Format (Lands of Lore)
 * @see https://moddingwiki.shikadi.net/wiki/Westwood_SHP_Format_(Lands_of_Lore)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_WESTWOOD_SHP_LANDS_OF_LORE_H
#define DUMPFLOPPY_FORMATS_SK_WESTWOOD_SHP_LANDS_OF_LORE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_westwood_shp_lands_of_lore final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SHP LOL";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Westwood_SHP_Format_(Lands_of_Lore)";
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
