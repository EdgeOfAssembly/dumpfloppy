/**
 * @file sk_ins_halloween_harry.h
 * @brief INS Format (Halloween Harry)
 * @see https://moddingwiki.shikadi.net/wiki/INS_Format_(Halloween_Harry)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_INS_HALLOWEEN_HARRY_H
#define DUMPFLOPPY_FORMATS_SK_INS_HALLOWEEN_HARRY_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_ins_halloween_harry final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "INS HARRY";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/INS_Format_(Halloween_Harry)";
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
