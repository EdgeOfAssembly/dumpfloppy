/**
 * @file sk_duke_1_demo.h
 * @brief Duke 1 Demo Format
 * @see https://moddingwiki.shikadi.net/wiki/Duke_1_Demo_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DUKE_1_DEMO_H
#define DUMPFLOPPY_FORMATS_SK_DUKE_1_DEMO_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_duke_1_demo final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DUKE 1 DEMO";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Duke_1_Demo_Format";
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
