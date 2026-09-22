/**
 * @file sk_dat_hocus_pocus.h
 * @brief DAT Format (Hocus Pocus)
 * @see https://moddingwiki.shikadi.net/wiki/DAT_Format_(Hocus_Pocus)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DAT_HOCUS_POCUS_H
#define DUMPFLOPPY_FORMATS_SK_DAT_HOCUS_POCUS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dat_hocus_pocus final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DAT HOCUS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/DAT_Format_(Hocus_Pocus)";
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
