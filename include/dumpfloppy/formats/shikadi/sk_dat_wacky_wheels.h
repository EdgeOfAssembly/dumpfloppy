/**
 * @file sk_dat_wacky_wheels.h
 * @brief DAT Format (Wacky Wheels)
 * @see https://moddingwiki.shikadi.net/wiki/DAT_Format_(Wacky_Wheels)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DAT_WACKY_WHEELS_H
#define DUMPFLOPPY_FORMATS_SK_DAT_WACKY_WHEELS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dat_wacky_wheels final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DAT WACKY";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/DAT_Format_(Wacky_Wheels)";
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
