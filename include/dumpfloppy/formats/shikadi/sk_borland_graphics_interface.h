/**
 * @file sk_borland_graphics_interface.h
 * @brief Borland Graphics Interface
 * @see https://moddingwiki.shikadi.net/wiki/Borland_Graphics_Interface
 */
#ifndef DUMPFLOPPY_FORMATS_SK_BORLAND_GRAPHICS_INTERFACE_H
#define DUMPFLOPPY_FORMATS_SK_BORLAND_GRAPHICS_INTERFACE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_borland_graphics_interface final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "BGI";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Borland_Graphics_Interface";
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
