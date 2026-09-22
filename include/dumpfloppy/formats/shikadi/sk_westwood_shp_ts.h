/**
 * @file sk_westwood_shp_ts.h
 * @brief Westwood SHP Format (TS)
 * @see https://moddingwiki.shikadi.net/wiki/Westwood_SHP_Format_(TS)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_WESTWOOD_SHP_TS_H
#define DUMPFLOPPY_FORMATS_SK_WESTWOOD_SHP_TS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_westwood_shp_ts final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SHP TS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Westwood_SHP_Format_(TS)";
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
