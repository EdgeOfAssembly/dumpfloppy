/**
 * @file sk_hi_zzt.h
 * @brief HI Format (ZZT)
 * @see https://moddingwiki.shikadi.net/wiki/HI_Format_(ZZT)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_HI_ZZT_H
#define DUMPFLOPPY_FORMATS_SK_HI_ZZT_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_hi_zzt final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "HI ZZT";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/HI_Format_(ZZT)";
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
