/**
 * @file sk_adlib_timbre_bank.h
 * @brief AdLib Timbre Bank Format
 * @see https://moddingwiki.shikadi.net/wiki/AdLib_Timbre_Bank_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_ADLIB_TIMBRE_BANK_H
#define DUMPFLOPPY_FORMATS_SK_ADLIB_TIMBRE_BANK_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_adlib_timbre_bank final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "ADLIB TIMBRE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/AdLib_Timbre_Bank_Format";
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
