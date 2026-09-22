/**
 * @file sk_wacky_wheels_high_score_files.h
 * @brief Wacky Wheels High Score Files
 * @see https://moddingwiki.shikadi.net/wiki/Wacky_Wheels_High_Score_Files
 */
#ifndef DUMPFLOPPY_FORMATS_SK_WACKY_WHEELS_HIGH_SCORE_FILES_H
#define DUMPFLOPPY_FORMATS_SK_WACKY_WHEELS_HIGH_SCORE_FILES_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_wacky_wheels_high_score_files final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "WACKY FILES";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Wacky_Wheels_High_Score_Files";
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
