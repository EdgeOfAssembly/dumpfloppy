/**
 * @file sk_hi_octane_texture_atlas.h
 * @brief Hi-Octane Texture Atlas
 * @see https://moddingwiki.shikadi.net/wiki/Hi-Octane_Texture_Atlas
 */
#ifndef DUMPFLOPPY_FORMATS_SK_HI_OCTANE_TEXTURE_ATLAS_H
#define DUMPFLOPPY_FORMATS_SK_HI_OCTANE_TEXTURE_ATLAS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_hi_octane_texture_atlas final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "HI-OCTANE ATLAS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Hi-Octane_Texture_Atlas";
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
