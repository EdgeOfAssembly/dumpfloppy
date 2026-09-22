/**
 * @file sk_b800_text.h
 * @brief B800 Text
 * @see https://moddingwiki.shikadi.net/wiki/B800_Text
 */
#ifndef DUMPFLOPPY_FORMATS_SK_B800_TEXT_H
#define DUMPFLOPPY_FORMATS_SK_B800_TEXT_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_b800_text final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "B800 TEXT";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/B800_Text";
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
