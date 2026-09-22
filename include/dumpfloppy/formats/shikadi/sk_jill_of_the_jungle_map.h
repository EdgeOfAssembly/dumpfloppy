/**
 * @file sk_jill_of_the_jungle_map.h
 * @brief Jill of the Jungle Map Format
 * @see https://moddingwiki.shikadi.net/wiki/Jill_of_the_Jungle_Map_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_JILL_OF_THE_JUNGLE_MAP_H
#define DUMPFLOPPY_FORMATS_SK_JILL_OF_THE_JUNGLE_MAP_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_jill_of_the_jungle_map final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "JILL JUNGLE MAP";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Jill_of_the_Jungle_Map_Format";
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
