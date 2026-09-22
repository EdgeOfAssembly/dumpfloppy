/**
 * @file sk_pak_the_learning_company.h
 * @brief PAK Format (The Learning Company)
 * @see https://moddingwiki.shikadi.net/wiki/PAK_Format_(The_Learning_Company)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_PAK_THE_LEARNING_COMPANY_H
#define DUMPFLOPPY_FORMATS_SK_PAK_THE_LEARNING_COMPANY_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_pak_the_learning_company final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "PAK TLC";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/PAK_Format_(The_Learning_Company)";
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
