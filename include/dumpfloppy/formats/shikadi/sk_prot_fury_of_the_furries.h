/**
 * @file sk_prot_fury_of_the_furries.h
 * @brief PROT Format (Fury of the Furries)
 * @see https://moddingwiki.shikadi.net/wiki/PROT_Format_(Fury_of_the_Furries)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_PROT_FURY_OF_THE_FURRIES_H
#define DUMPFLOPPY_FORMATS_SK_PROT_FURY_OF_THE_FURRIES_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_prot_fury_of_the_furries final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "PROT FOTF";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/PROT_Format_(Fury_of_the_Furries)";
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
