/**
 * @file sk_swords_of_glass_info.h
 * @brief Swords of Glass info format
 * @see https://moddingwiki.shikadi.net/wiki/Swords_of_Glass_info_format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_SWORDS_OF_GLASS_INFO_H
#define DUMPFLOPPY_FORMATS_SK_SWORDS_OF_GLASS_INFO_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_swords_of_glass_info final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SWORDS INFO";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Swords_of_Glass_info_format";
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
