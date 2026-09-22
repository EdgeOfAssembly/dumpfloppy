/**
 * @file sk_gem_font.h
 * @brief GEM Font
 * @see https://moddingwiki.shikadi.net/wiki/GEM_Font
 */
#ifndef DUMPFLOPPY_FORMATS_SK_GEM_FONT_H
#define DUMPFLOPPY_FORMATS_SK_GEM_FONT_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_gem_font final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "GEM FONT";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/GEM_Font";
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
