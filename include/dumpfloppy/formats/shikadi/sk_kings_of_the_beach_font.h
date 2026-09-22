/**
 * @file sk_kings_of_the_beach_font.h
 * @brief Kings of the Beach Font
 * @see https://moddingwiki.shikadi.net/wiki/Kings_of_the_Beach_Font
 */
#ifndef DUMPFLOPPY_FORMATS_SK_KINGS_OF_THE_BEACH_FONT_H
#define DUMPFLOPPY_FORMATS_SK_KINGS_OF_THE_BEACH_FONT_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_kings_of_the_beach_font final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "KINGS BEACH FONT";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Kings_of_the_Beach_Font";
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
