/**
 * @file sk_raptor_animated_graphics.h
 * @brief Raptor Animated Graphics
 * @see https://moddingwiki.shikadi.net/wiki/Raptor_Animated_Graphics
 */
#ifndef DUMPFLOPPY_FORMATS_SK_RAPTOR_ANIMATED_GRAPHICS_H
#define DUMPFLOPPY_FORMATS_SK_RAPTOR_ANIMATED_GRAPHICS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_raptor_animated_graphics final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "RAPTOR GRAPHICS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Raptor_Animated_Graphics";
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
