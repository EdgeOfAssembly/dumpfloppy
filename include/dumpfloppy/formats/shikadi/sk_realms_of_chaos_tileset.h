/**
 * @file sk_realms_of_chaos_tileset.h
 * @brief Realms of Chaos Tileset Format
 * @see https://moddingwiki.shikadi.net/wiki/Realms_of_Chaos_Tileset_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_REALMS_OF_CHAOS_TILESET_H
#define DUMPFLOPPY_FORMATS_SK_REALMS_OF_CHAOS_TILESET_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_realms_of_chaos_tileset final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "REALMS TILESET";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Realms_of_Chaos_Tileset_Format";
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
