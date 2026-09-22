/**
 * @file sk_cdfm.h
 * @brief CDFM Format
 * @see https://moddingwiki.shikadi.net/wiki/CDFM_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CDFM_H
#define DUMPFLOPPY_FORMATS_SK_CDFM_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_cdfm final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CDFM";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/CDFM_Format";
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
