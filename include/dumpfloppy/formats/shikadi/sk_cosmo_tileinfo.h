/**
 * @file sk_cosmo_tileinfo.h
 * @brief Cosmo Tileinfo Format
 * @see https://moddingwiki.shikadi.net/wiki/Cosmo_Tileinfo_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_COSMO_TILEINFO_H
#define DUMPFLOPPY_FORMATS_SK_COSMO_TILEINFO_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_cosmo_tileinfo final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "COSMO TILEINFO";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Cosmo_Tileinfo_Format";
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
