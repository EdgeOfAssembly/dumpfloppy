/**
 * @file sk_sango_fighter_rlc_compression.h
 * @brief Sango Fighter RLC Compression
 * @see https://moddingwiki.shikadi.net/wiki/Sango_Fighter_RLC_Compression
 */
#ifndef DUMPFLOPPY_FORMATS_SK_SANGO_FIGHTER_RLC_COMPRESSION_H
#define DUMPFLOPPY_FORMATS_SK_SANGO_FIGHTER_RLC_COMPRESSION_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_sango_fighter_rlc_compression final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SANGO RLC";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Sango_Fighter_RLC_Compression";
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
