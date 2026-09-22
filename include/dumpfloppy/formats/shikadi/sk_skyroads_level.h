/**
 * @file sk_skyroads_level.h
 * @brief SkyRoads level format
 * @see https://moddingwiki.shikadi.net/wiki/SkyRoads_level_format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_SKYROADS_LEVEL_H
#define DUMPFLOPPY_FORMATS_SK_SKYROADS_LEVEL_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_skyroads_level final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SKYROADS LEVEL";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/SkyRoads_level_format";
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
