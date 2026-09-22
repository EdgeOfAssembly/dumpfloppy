/**
 * @file sk_the_blues_brothers_map.h
 * @brief The Blues Brothers Map Format
 * @see https://moddingwiki.shikadi.net/wiki/The_Blues_Brothers_Map_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_THE_BLUES_BROTHERS_MAP_H
#define DUMPFLOPPY_FORMATS_SK_THE_BLUES_BROTHERS_MAP_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_the_blues_brothers_map final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "BLUES MAP";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/The_Blues_Brothers_Map_Format";
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
