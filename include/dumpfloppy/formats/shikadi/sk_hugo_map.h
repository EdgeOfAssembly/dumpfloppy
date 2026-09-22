/**
 * @file sk_hugo_map.h
 * @brief Hugo Map Formats
 * @see https://moddingwiki.shikadi.net/wiki/Hugo_Map_Formats
 */
#ifndef DUMPFLOPPY_FORMATS_SK_HUGO_MAP_H
#define DUMPFLOPPY_FORMATS_SK_HUGO_MAP_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_hugo_map final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "HUGO MAP";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Hugo_Map_Formats";
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
