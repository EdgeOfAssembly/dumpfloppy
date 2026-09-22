/**
 * @file sk_commander_keen_1_3_score.h
 * @brief Commander Keen 1-3 Score format
 * @see https://moddingwiki.shikadi.net/wiki/Commander_Keen_1-3_Score_format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_COMMANDER_KEEN_1_3_SCORE_H
#define DUMPFLOPPY_FORMATS_SK_COMMANDER_KEEN_1_3_SCORE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_commander_keen_1_3_score final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "KEEN SCORE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Commander_Keen_1-3_Score_format";
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
