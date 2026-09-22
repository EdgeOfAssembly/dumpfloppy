/**
 * @file sk_jazz_jackrabbit_planet.h
 * @brief Jazz Jackrabbit Planet Format
 * @see https://moddingwiki.shikadi.net/wiki/Jazz_Jackrabbit_Planet_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_JAZZ_JACKRABBIT_PLANET_H
#define DUMPFLOPPY_FORMATS_SK_JAZZ_JACKRABBIT_PLANET_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_jazz_jackrabbit_planet final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "JAZZ PLANET";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Jazz_Jackrabbit_Planet_Format";
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
