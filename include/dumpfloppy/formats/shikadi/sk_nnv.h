/**
 * @file sk_nnv.h
 * @brief NNV Format
 * @see https://moddingwiki.shikadi.net/wiki/NNV_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_NNV_H
#define DUMPFLOPPY_FORMATS_SK_NNV_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_nnv final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "NNV";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/NNV_Format";
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
