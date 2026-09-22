/**
 * @file sk_raw_ega_data.h
 * @brief Raw EGA data
 * @see https://moddingwiki.shikadi.net/wiki/Raw_EGA_data
 */
#ifndef DUMPFLOPPY_FORMATS_SK_RAW_EGA_DATA_H
#define DUMPFLOPPY_FORMATS_SK_RAW_EGA_DATA_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_raw_ega_data final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "RAW EGA";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Raw_EGA_data";
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
