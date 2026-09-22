/**
 * @file sk_commander_keen_ega_header.h
 * @brief Commander Keen EGA Header
 * @see https://moddingwiki.shikadi.net/wiki/Commander_Keen_EGA_Header
 */
#ifndef DUMPFLOPPY_FORMATS_SK_COMMANDER_KEEN_EGA_HEADER_H
#define DUMPFLOPPY_FORMATS_SK_COMMANDER_KEEN_EGA_HEADER_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_commander_keen_ega_header final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "KEEN EGAHEAD";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Commander_Keen_EGA_Header";
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
