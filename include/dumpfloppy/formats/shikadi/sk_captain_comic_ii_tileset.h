/**
 * @file sk_captain_comic_ii_tileset.h
 * @brief Captain Comic II Tileset Format
 * @see https://moddingwiki.shikadi.net/wiki/Captain_Comic_II_Tileset_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CAPTAIN_COMIC_II_TILESET_H
#define DUMPFLOPPY_FORMATS_SK_CAPTAIN_COMIC_II_TILESET_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_captain_comic_ii_tileset final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CAPTAIN TILESET";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Captain_Comic_II_Tileset_Format";
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
