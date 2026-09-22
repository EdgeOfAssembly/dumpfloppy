/**
 * @file sk_kings_of_the_beach_pak.h
 * @brief Kings of the Beach PAK Format
 * @see https://moddingwiki.shikadi.net/wiki/Kings_of_the_Beach_PAK_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_KINGS_OF_THE_BEACH_PAK_H
#define DUMPFLOPPY_FORMATS_SK_KINGS_OF_THE_BEACH_PAK_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_kings_of_the_beach_pak final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "KINGS BEACH PAK";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Kings_of_the_Beach_PAK_Format";
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
