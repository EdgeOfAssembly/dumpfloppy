/**
 * @file sk_family_feud_question_flag.h
 * @brief Family Feud Question Flag Format
 * @see https://moddingwiki.shikadi.net/wiki/Family_Feud_Question_Flag_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_FAMILY_FEUD_QUESTION_FLAG_H
#define DUMPFLOPPY_FORMATS_SK_FAMILY_FEUD_QUESTION_FLAG_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_family_feud_question_flag final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "FF Q FLAG";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Family_Feud_Question_Flag_Format";
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
