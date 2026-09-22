/**
 * @file sk_cc1.h
 * @brief CC1 Format
 * @see https://moddingwiki.shikadi.net/wiki/CC1_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CC1_H
#define DUMPFLOPPY_FORMATS_SK_CC1_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_cc1 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CC1";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/CC1_Format";
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
