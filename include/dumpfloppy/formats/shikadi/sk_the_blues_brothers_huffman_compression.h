/**
 * @file sk_the_blues_brothers_huffman_compression.h
 * @brief The Blues Brothers Huffman Compression
 * @see https://moddingwiki.shikadi.net/wiki/The_Blues_Brothers_Huffman_Compression
 */
#ifndef DUMPFLOPPY_FORMATS_SK_THE_BLUES_BROTHERS_HUFFMAN_COMPRESSION_H
#define DUMPFLOPPY_FORMATS_SK_THE_BLUES_BROTHERS_HUFFMAN_COMPRESSION_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_the_blues_brothers_huffman_compression final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "BB HUFFMAN";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/The_Blues_Brothers_Huffman_Compression";
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
