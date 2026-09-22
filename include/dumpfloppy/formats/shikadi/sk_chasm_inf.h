/**
 * @file sk_chasm_inf.h
 * @brief CHASM.INF
 * @see https://moddingwiki.shikadi.net/wiki/CHASM.INF
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CHASM_INF_H
#define DUMPFLOPPY_FORMATS_SK_CHASM_INF_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_chasm_inf final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CHASM.INF";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/CHASM.INF";
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
