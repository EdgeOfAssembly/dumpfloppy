/**
 * @file sk_pref_dat_witchaven.h
 * @brief PREF.DAT (Witchaven)
 * @see https://moddingwiki.shikadi.net/wiki/PREF.DAT_(Witchaven)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_PREF_DAT_WITCHAVEN_H
#define DUMPFLOPPY_FORMATS_SK_PREF_DAT_WITCHAVEN_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_pref_dat_witchaven final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "PREF.DAT";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/PREF.DAT_(Witchaven)";
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
