/**
 * @file sk_westwood_xor_delta.h
 * @brief Westwood XOR Delta
 * @see https://moddingwiki.shikadi.net/wiki/Westwood_XOR_Delta
 */
#ifndef DUMPFLOPPY_FORMATS_SK_WESTWOOD_XOR_DELTA_H
#define DUMPFLOPPY_FORMATS_SK_WESTWOOD_XOR_DELTA_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_westwood_xor_delta final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "WW XOR DELTA";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Westwood_XOR_Delta";
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
