/**
 * @file sk_rts.h
 * @brief RTS Format
 * @see https://moddingwiki.shikadi.net/wiki/RTS_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_RTS_H
#define DUMPFLOPPY_FORMATS_SK_RTS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_rts final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "RTS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/RTS_Format";
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
