/**
 * @file sk_westwood_wsa.h
 * @brief Westwood WSA Format
 * @see https://moddingwiki.shikadi.net/wiki/Westwood_WSA_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_WESTWOOD_WSA_H
#define DUMPFLOPPY_FORMATS_SK_WESTWOOD_WSA_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_westwood_wsa final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "WW WSA";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Westwood_WSA_Format";
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
