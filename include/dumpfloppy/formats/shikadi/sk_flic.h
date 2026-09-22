/**
 * @file sk_flic.h
 * @brief FLIC Format
 * @see https://moddingwiki.shikadi.net/wiki/FLIC_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_FLIC_H
#define DUMPFLOPPY_FORMATS_SK_FLIC_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_flic final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "FLIC";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/FLIC_Format";
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
