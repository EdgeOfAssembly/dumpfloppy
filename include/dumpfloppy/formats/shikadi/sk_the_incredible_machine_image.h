/**
 * @file sk_the_incredible_machine_image.h
 * @brief The Incredible Machine Image Format
 * @see https://moddingwiki.shikadi.net/wiki/The_Incredible_Machine_Image_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_THE_INCREDIBLE_MACHINE_IMAGE_H
#define DUMPFLOPPY_FORMATS_SK_THE_INCREDIBLE_MACHINE_IMAGE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_the_incredible_machine_image final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "TIM IMAGE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/The_Incredible_Machine_Image_Format";
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
