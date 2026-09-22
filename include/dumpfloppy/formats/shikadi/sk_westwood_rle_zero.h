/**
 * @file sk_westwood_rle_zero.h
 * @brief Westwood RLE-Zero
 * @see https://moddingwiki.shikadi.net/wiki/Westwood_RLE-Zero
 */
#ifndef DUMPFLOPPY_FORMATS_SK_WESTWOOD_RLE_ZERO_H
#define DUMPFLOPPY_FORMATS_SK_WESTWOOD_RLE_ZERO_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_westwood_rle_zero final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "WW RLE-ZERO";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Westwood_RLE-Zero";
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
