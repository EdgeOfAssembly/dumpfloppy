/**
 * @file sk_secret_agent_level.h
 * @brief Secret Agent Level Format
 * @see https://moddingwiki.shikadi.net/wiki/Secret_Agent_Level_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_SECRET_AGENT_LEVEL_H
#define DUMPFLOPPY_FORMATS_SK_SECRET_AGENT_LEVEL_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_secret_agent_level final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SECRET LEVEL";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Secret_Agent_Level_Format";
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
