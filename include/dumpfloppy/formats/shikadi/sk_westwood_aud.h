/**
 * @file sk_westwood_aud.h
 * @brief Westwood AUD Format
 * @see https://moddingwiki.shikadi.net/wiki/Westwood_AUD_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_WESTWOOD_AUD_H
#define DUMPFLOPPY_FORMATS_SK_WESTWOOD_AUD_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_westwood_aud final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "WW AUD";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Westwood_AUD_Format";
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
