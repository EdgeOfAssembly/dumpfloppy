/**
 * @file sk_ddici_graphics.h
 * @brief DDiCI Graphics format
 * @see https://moddingwiki.shikadi.net/wiki/DDiCI_Graphics_format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DDICI_GRAPHICS_H
#define DUMPFLOPPY_FORMATS_SK_DDICI_GRAPHICS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_ddici_graphics final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DDICI GRAPHICS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/DDiCI_Graphics_format";
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
