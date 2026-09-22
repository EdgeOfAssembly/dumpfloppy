/**
 * @file sk_westwood_lzw_12.h
 * @brief Westwood LZW-12
 * @see https://moddingwiki.shikadi.net/wiki/Westwood_LZW-12
 */
#ifndef DUMPFLOPPY_FORMATS_SK_WESTWOOD_LZW_12_H
#define DUMPFLOPPY_FORMATS_SK_WESTWOOD_LZW_12_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_westwood_lzw_12 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "WW LZW-12";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Westwood_LZW-12";
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
