/**
 * @file sk_map_shadowcaster.h
 * @brief MAP Format (Shadowcaster)
 * @see https://moddingwiki.shikadi.net/wiki/MAP_Format_(Shadowcaster)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_MAP_SHADOWCASTER_H
#define DUMPFLOPPY_FORMATS_SK_MAP_SHADOWCASTER_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_map_shadowcaster final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "MAP SHADOW";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/MAP_Format_(Shadowcaster)";
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
