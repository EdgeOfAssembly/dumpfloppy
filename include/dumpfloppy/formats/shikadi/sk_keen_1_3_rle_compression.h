/**
 * @file sk_keen_1_3_rle_compression.h
 * @brief Keen 1-3 RLE compression
 * @see https://moddingwiki.shikadi.net/wiki/Keen_1-3_RLE_compression
 */
#ifndef DUMPFLOPPY_FORMATS_SK_KEEN_1_3_RLE_COMPRESSION_H
#define DUMPFLOPPY_FORMATS_SK_KEEN_1_3_RLE_COMPRESSION_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_keen_1_3_rle_compression final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "KEEN RLE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Keen_1-3_RLE_compression";
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
