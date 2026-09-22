/**
 * @file sk_zone_66_tileset.h
 * @brief Zone 66 Tileset Format
 * @see https://moddingwiki.shikadi.net/wiki/Zone_66_Tileset_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_ZONE_66_TILESET_H
#define DUMPFLOPPY_FORMATS_SK_ZONE_66_TILESET_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_zone_66_tileset final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "ZONE 66 TILESET";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Zone_66_Tileset_Format";
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
