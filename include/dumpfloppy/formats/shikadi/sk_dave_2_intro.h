/**
 * @file sk_dave_2_intro.h
 * @brief Dave 2 Intro format
 * @see https://moddingwiki.shikadi.net/wiki/Dave_2_Intro_format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DAVE_2_INTRO_H
#define DUMPFLOPPY_FORMATS_SK_DAVE_2_INTRO_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dave_2_intro final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DAVE 2 INTRO";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Dave_2_Intro_format";
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
