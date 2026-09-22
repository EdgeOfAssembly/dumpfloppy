/**
 * @file sk_jazz_jackrabbit_score.h
 * @brief Jazz Jackrabbit Score Format
 * @see https://moddingwiki.shikadi.net/wiki/Jazz_Jackrabbit_Score_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_JAZZ_JACKRABBIT_SCORE_H
#define DUMPFLOPPY_FORMATS_SK_JAZZ_JACKRABBIT_SCORE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_jazz_jackrabbit_score final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "JAZZ SCORE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Jazz_Jackrabbit_Score_Format";
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
