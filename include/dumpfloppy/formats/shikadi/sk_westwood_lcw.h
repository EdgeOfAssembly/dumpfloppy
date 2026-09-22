/**
 * @file sk_westwood_lcw.h
 * @brief Westwood LCW
 * @see https://moddingwiki.shikadi.net/wiki/Westwood_LCW
 */
#ifndef DUMPFLOPPY_FORMATS_SK_WESTWOOD_LCW_H
#define DUMPFLOPPY_FORMATS_SK_WESTWOOD_LCW_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_westwood_lcw final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "WW LCW";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Westwood_LCW";
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
