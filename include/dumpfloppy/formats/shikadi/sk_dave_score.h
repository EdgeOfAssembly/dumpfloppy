/**
 * @file sk_dave_score.h
 * @brief Dave Score format
 * @see https://moddingwiki.shikadi.net/wiki/Dave_Score_format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DAVE_SCORE_H
#define DUMPFLOPPY_FORMATS_SK_DAVE_SCORE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dave_score final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DAVE SCORE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Dave_Score_format";
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
