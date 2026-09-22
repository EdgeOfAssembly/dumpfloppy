/**
 * @file sk_lion_king_level.h
 * @brief Lion King Level Format
 * @see https://moddingwiki.shikadi.net/wiki/Lion_King_Level_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_LION_KING_LEVEL_H
#define DUMPFLOPPY_FORMATS_SK_LION_KING_LEVEL_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_lion_king_level final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "LION KING LEVEL";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Lion_King_Level_Format";
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
