/**
 * @file sk_westwood_shp_td.h
 * @brief Westwood SHP Format (TD)
 * @see https://moddingwiki.shikadi.net/wiki/Westwood_SHP_Format_(TD)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_WESTWOOD_SHP_TD_H
#define DUMPFLOPPY_FORMATS_SK_WESTWOOD_SHP_TD_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_westwood_shp_td final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SHP TD";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Westwood_SHP_Format_(TD)";
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
