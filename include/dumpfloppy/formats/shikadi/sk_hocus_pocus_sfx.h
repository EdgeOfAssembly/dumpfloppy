/**
 * @file sk_hocus_pocus_sfx.h
 * @brief Hocus Pocus SFX Format
 * @see https://moddingwiki.shikadi.net/wiki/Hocus_Pocus_SFX_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_HOCUS_POCUS_SFX_H
#define DUMPFLOPPY_FORMATS_SK_HOCUS_POCUS_SFX_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_hocus_pocus_sfx final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "HOCUS POCUS SFX";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Hocus_Pocus_SFX_Format";
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
