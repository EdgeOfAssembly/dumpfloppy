/**
 * @file sk_carmack_compression.h
 * @brief Carmack compression
 * @see https://moddingwiki.shikadi.net/wiki/Carmack_compression
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CARMACK_COMPRESSION_H
#define DUMPFLOPPY_FORMATS_SK_CARMACK_COMPRESSION_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_carmack_compression final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CARMACK";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Carmack_compression";
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
