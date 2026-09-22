/**
 * @file sk_castle_adventure_data.h
 * @brief Castle Adventure Data Format
 * @see https://moddingwiki.shikadi.net/wiki/Castle_Adventure_Data_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CASTLE_ADVENTURE_DATA_H
#define DUMPFLOPPY_FORMATS_SK_CASTLE_ADVENTURE_DATA_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_castle_adventure_data final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CASTLE DATA";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Castle_Adventure_Data_Format";
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
