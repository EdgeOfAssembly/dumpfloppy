/**
 * @file sk_raw_cga_data.h
 * @brief Raw CGA Data
 * @see https://moddingwiki.shikadi.net/wiki/Raw_CGA_Data
 */
#ifndef DUMPFLOPPY_FORMATS_SK_RAW_CGA_DATA_H
#define DUMPFLOPPY_FORMATS_SK_RAW_CGA_DATA_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_raw_cga_data final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "RAW CGA";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Raw_CGA_Data";
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
