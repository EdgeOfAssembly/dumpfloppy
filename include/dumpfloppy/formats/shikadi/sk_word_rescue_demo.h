/**
 * @file sk_word_rescue_demo.h
 * @brief Word Rescue Demo Format
 * @see https://moddingwiki.shikadi.net/wiki/Word_Rescue_Demo_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_WORD_RESCUE_DEMO_H
#define DUMPFLOPPY_FORMATS_SK_WORD_RESCUE_DEMO_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_word_rescue_demo final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "WORD RESCUE DEMO";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Word_Rescue_Demo_Format";
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
