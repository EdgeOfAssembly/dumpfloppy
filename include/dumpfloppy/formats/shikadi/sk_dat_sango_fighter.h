/**
 * @file sk_dat_sango_fighter.h
 * @brief DAT Format (Sango Fighter)
 * @see https://moddingwiki.shikadi.net/wiki/DAT_Format_(Sango_Fighter)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DAT_SANGO_FIGHTER_H
#define DUMPFLOPPY_FORMATS_SK_DAT_SANGO_FIGHTER_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dat_sango_fighter final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DAT SANGO";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/DAT_Format_(Sango_Fighter)";
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
