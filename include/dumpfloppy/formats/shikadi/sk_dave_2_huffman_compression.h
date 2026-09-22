/**
 * @file sk_dave_2_huffman_compression.h
 * @brief Dave 2 Huffman compression
 * @see https://moddingwiki.shikadi.net/wiki/Dave_2_Huffman_compression
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DAVE_2_HUFFMAN_COMPRESSION_H
#define DUMPFLOPPY_FORMATS_SK_DAVE_2_HUFFMAN_COMPRESSION_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dave_2_huffman_compression final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DAVE2 HUFFMAN";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Dave_2_Huffman_compression";
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
