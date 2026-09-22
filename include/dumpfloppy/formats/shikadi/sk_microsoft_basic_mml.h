/**
 * @file sk_microsoft_basic_mml.h
 * @brief Microsoft BASIC MML
 * @see https://moddingwiki.shikadi.net/wiki/Microsoft_BASIC_MML
 */
#ifndef DUMPFLOPPY_FORMATS_SK_MICROSOFT_BASIC_MML_H
#define DUMPFLOPPY_FORMATS_SK_MICROSOFT_BASIC_MML_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_microsoft_basic_mml final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "BASIC MML";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Microsoft_BASIC_MML";
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
