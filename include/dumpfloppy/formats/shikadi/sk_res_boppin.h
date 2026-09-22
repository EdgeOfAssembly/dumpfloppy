/**
 * @file sk_res_boppin.h
 * @brief RES Format (Boppin)
 * @see https://moddingwiki.shikadi.net/wiki/RES_Format_(Boppin)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_RES_BOPPIN_H
#define DUMPFLOPPY_FORMATS_SK_RES_BOPPIN_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_res_boppin final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "RES BOPPIN";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/RES_Format_(Boppin)";
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
