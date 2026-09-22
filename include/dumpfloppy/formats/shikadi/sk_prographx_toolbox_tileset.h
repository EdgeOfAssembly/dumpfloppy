/**
 * @file sk_prographx_toolbox_tileset.h
 * @brief ProGraphx Toolbox tileset format
 * @see https://moddingwiki.shikadi.net/wiki/ProGraphx_Toolbox_tileset_format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_PROGRAPHX_TOOLBOX_TILESET_H
#define DUMPFLOPPY_FORMATS_SK_PROGRAPHX_TOOLBOX_TILESET_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_prographx_toolbox_tileset final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "PROGRAPHX TILESE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/ProGraphx_Toolbox_tileset_format";
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
