/**
 * @file sk_commander_keen_1_3_level.h
 * @brief Commander Keen 1-3 Level format
 * @see https://moddingwiki.shikadi.net/wiki/Commander_Keen_1-3_Level_format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_COMMANDER_KEEN_1_3_LEVEL_H
#define DUMPFLOPPY_FORMATS_SK_COMMANDER_KEEN_1_3_LEVEL_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_commander_keen_1_3_level final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "KEEN MAP";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Commander_Keen_1-3_Level_format";
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
