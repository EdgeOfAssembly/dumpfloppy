/**
 * @file sk_catacomb_tileset.h
 * @brief Catacomb Tileset Format
 * @see https://moddingwiki.shikadi.net/wiki/Catacomb_Tileset_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CATACOMB_TILESET_H
#define DUMPFLOPPY_FORMATS_SK_CATACOMB_TILESET_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_catacomb_tileset final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CATACOMB TILESET";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Catacomb_Tileset_Format";
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
