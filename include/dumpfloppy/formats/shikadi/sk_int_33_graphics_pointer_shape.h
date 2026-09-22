/**
 * @file sk_int_33_graphics_pointer_shape.h
 * @brief INT 33 Graphics Pointer Shape
 * @see https://moddingwiki.shikadi.net/wiki/INT_33_Graphics_Pointer_Shape
 */
#ifndef DUMPFLOPPY_FORMATS_SK_INT_33_GRAPHICS_POINTER_SHAPE_H
#define DUMPFLOPPY_FORMATS_SK_INT_33_GRAPHICS_POINTER_SHAPE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_int_33_graphics_pointer_shape final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "INT33 CURSOR";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/INT_33_Graphics_Pointer_Shape";
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
