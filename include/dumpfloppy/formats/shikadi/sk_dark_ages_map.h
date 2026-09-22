/**
 * @file sk_dark_ages_map.h
 * @brief Dark Ages Map Format
 * @see https://moddingwiki.shikadi.net/wiki/Dark_Ages_Map_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DARK_AGES_MAP_H
#define DUMPFLOPPY_FORMATS_SK_DARK_AGES_MAP_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dark_ages_map final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DARK AGES MAP";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Dark_Ages_Map_Format";
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
