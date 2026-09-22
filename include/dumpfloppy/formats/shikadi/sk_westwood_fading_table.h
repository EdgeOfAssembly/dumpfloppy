/**
 * @file sk_westwood_fading_table.h
 * @brief Westwood Fading Table
 * @see https://moddingwiki.shikadi.net/wiki/Westwood_Fading_Table
 */
#ifndef DUMPFLOPPY_FORMATS_SK_WESTWOOD_FADING_TABLE_H
#define DUMPFLOPPY_FORMATS_SK_WESTWOOD_FADING_TABLE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_westwood_fading_table final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "WW FADE TBL";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Westwood_Fading_Table";
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
