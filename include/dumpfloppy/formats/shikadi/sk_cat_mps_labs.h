/**
 * @file sk_cat_mps_labs.h
 * @brief CAT Format (MPS Labs)
 * @see https://moddingwiki.shikadi.net/wiki/CAT_Format_(MPS_Labs)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CAT_MPS_LABS_H
#define DUMPFLOPPY_FORMATS_SK_CAT_MPS_LABS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_cat_mps_labs final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CAT MPS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/CAT_Format_(MPS_Labs)";
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
