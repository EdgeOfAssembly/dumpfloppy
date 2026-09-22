/**
 * @file sk_rob_northern_compression.h
 * @brief Rob Northern Compression
 * @see https://moddingwiki.shikadi.net/wiki/Rob_Northern_Compression
 */
#ifndef DUMPFLOPPY_FORMATS_SK_ROB_NORTHERN_COMPRESSION_H
#define DUMPFLOPPY_FORMATS_SK_ROB_NORTHERN_COMPRESSION_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_rob_northern_compression final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "RNC";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Rob_Northern_Compression";
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
