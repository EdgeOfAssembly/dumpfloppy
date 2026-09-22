/**
 * @file sk_zzt.h
 * @brief ZZT Format
 * @see https://moddingwiki.shikadi.net/wiki/ZZT_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_ZZT_H
#define DUMPFLOPPY_FORMATS_SK_ZZT_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_zzt final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "ZZT";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/ZZT_Format";
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
