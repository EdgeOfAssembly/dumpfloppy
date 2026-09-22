/**
 * @file sk_dat_highway_hunter.h
 * @brief DAT Format (Highway Hunter)
 * @see https://moddingwiki.shikadi.net/wiki/DAT_Format_(Highway_Hunter)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DAT_HIGHWAY_HUNTER_H
#define DUMPFLOPPY_FORMATS_SK_DAT_HIGHWAY_HUNTER_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dat_highway_hunter final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DAT HWY HUNTER";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/DAT_Format_(Highway_Hunter)";
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
