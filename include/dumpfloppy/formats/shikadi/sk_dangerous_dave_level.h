/**
 * @file sk_dangerous_dave_level.h
 * @brief Dangerous Dave Level format
 * @see https://moddingwiki.shikadi.net/wiki/Dangerous_Dave_Level_format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DANGEROUS_DAVE_LEVEL_H
#define DUMPFLOPPY_FORMATS_SK_DANGEROUS_DAVE_LEVEL_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dangerous_dave_level final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DANGEROUS LEVEL";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Dangerous_Dave_Level_format";
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
