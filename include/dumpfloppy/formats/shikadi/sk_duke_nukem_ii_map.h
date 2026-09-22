/**
 * @file sk_duke_nukem_ii_map.h
 * @brief Duke Nukem II Map Format
 * @see https://moddingwiki.shikadi.net/wiki/Duke_Nukem_II_Map_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DUKE_NUKEM_II_MAP_H
#define DUMPFLOPPY_FORMATS_SK_DUKE_NUKEM_II_MAP_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_duke_nukem_ii_map final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DUKE MAP";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Duke_Nukem_II_Map_Format";
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
