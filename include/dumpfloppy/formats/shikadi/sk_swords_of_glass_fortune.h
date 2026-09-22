/**
 * @file sk_swords_of_glass_fortune.h
 * @brief Swords of Glass fortune format
 * @see https://moddingwiki.shikadi.net/wiki/Swords_of_Glass_fortune_format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_SWORDS_OF_GLASS_FORTUNE_H
#define DUMPFLOPPY_FORMATS_SK_SWORDS_OF_GLASS_FORTUNE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_swords_of_glass_fortune final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SWORDS FORTUNE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Swords_of_Glass_fortune_format";
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
