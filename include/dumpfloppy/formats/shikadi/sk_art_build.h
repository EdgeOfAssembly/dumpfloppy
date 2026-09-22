/**
 * @file sk_art_build.h
 * @brief ART Format (Build)
 * @see https://moddingwiki.shikadi.net/wiki/ART_Format_(Build)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_ART_BUILD_H
#define DUMPFLOPPY_FORMATS_SK_ART_BUILD_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_art_build final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "ART BUILD";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/ART_Format_(Build)";
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
