/**
 * @file sk_realms_of_chaos_level.h
 * @brief Realms of Chaos Level Format
 * @see https://moddingwiki.shikadi.net/wiki/Realms_of_Chaos_Level_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_REALMS_OF_CHAOS_LEVEL_H
#define DUMPFLOPPY_FORMATS_SK_REALMS_OF_CHAOS_LEVEL_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_realms_of_chaos_level final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "REALMS LEVEL";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Realms_of_Chaos_Level_Format";
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
