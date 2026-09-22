/**
 * @file sk_shadow_warrior_1993_beta_image.h
 * @brief Shadow Warrior (1993 Beta) Image Format
 * @see https://moddingwiki.shikadi.net/wiki/Shadow_Warrior_(1993_Beta)_Image_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_SHADOW_WARRIOR_1993_BETA_IMAGE_H
#define DUMPFLOPPY_FORMATS_SK_SHADOW_WARRIOR_1993_BETA_IMAGE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_shadow_warrior_1993_beta_image final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SW93 IMAGE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Shadow_Warrior_(1993_Beta)_Image_Format";
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
