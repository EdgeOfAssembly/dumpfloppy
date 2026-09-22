/**
 * @file sk_dat_dr_riptide.h
 * @brief DAT Format (Dr. Riptide)
 * @see https://moddingwiki.shikadi.net/wiki/DAT_Format_(Dr._Riptide)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DAT_DR_RIPTIDE_H
#define DUMPFLOPPY_FORMATS_SK_DAT_DR_RIPTIDE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dat_dr_riptide final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DAT RIPTIDE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/DAT_Format_(Dr._Riptide)";
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
