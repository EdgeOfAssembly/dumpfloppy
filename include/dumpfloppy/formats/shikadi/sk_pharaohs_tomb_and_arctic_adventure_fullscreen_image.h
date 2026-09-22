/**
 * @file sk_pharaohs_tomb_and_arctic_adventure_fullscreen_image.h
 * @brief Pharaoh's Tomb & Arctic Adventure Fullscreen Image Format
 * @see https://moddingwiki.shikadi.net/wiki/Pharaoh's_Tomb_&_Arctic_Adventure_Fullscreen_Image_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_PHARAOHS_TOMB_AND_ARCTIC_ADVENTURE_FULLSCREEN_IMAGE_H
#define DUMPFLOPPY_FORMATS_SK_PHARAOHS_TOMB_AND_ARCTIC_ADVENTURE_FULLSCREEN_IMAGE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_pharaohs_tomb_and_arctic_adventure_fullscreen_image final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "PTAA FULL IMG";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Pharaoh's_Tomb_&_Arctic_Adventure_Fullscreen_Image_Format";
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
