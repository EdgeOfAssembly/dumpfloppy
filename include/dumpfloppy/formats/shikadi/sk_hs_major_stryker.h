/**
 * @file sk_hs_major_stryker.h
 * @brief HS Format (Major Stryker)
 * @see https://moddingwiki.shikadi.net/wiki/HS_Format_(Major_Stryker)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_HS_MAJOR_STRYKER_H
#define DUMPFLOPPY_FORMATS_SK_HS_MAJOR_STRYKER_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_hs_major_stryker final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "HS STRYKER";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/HS_Format_(Major_Stryker)";
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
