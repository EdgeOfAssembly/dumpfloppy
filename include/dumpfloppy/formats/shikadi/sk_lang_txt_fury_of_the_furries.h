/**
 * @file sk_lang_txt_fury_of_the_furries.h
 * @brief LANG.TXT Format (Fury of the Furries)
 * @see https://moddingwiki.shikadi.net/wiki/LANG.TXT_Format_(Fury_of_the_Furries)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_LANG_TXT_FURY_OF_THE_FURRIES_H
#define DUMPFLOPPY_FORMATS_SK_LANG_TXT_FURY_OF_THE_FURRIES_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_lang_txt_fury_of_the_furries final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "LANG.TXT";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/LANG.TXT_Format_(Fury_of_the_Furries)";
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
