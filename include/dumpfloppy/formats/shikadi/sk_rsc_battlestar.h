/**
 * @file sk_rsc_battlestar.h
 * @brief RSC Format (BattleStar)
 * @see https://moddingwiki.shikadi.net/wiki/RSC_Format_(BattleStar)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_RSC_BATTLESTAR_H
#define DUMPFLOPPY_FORMATS_SK_RSC_BATTLESTAR_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_rsc_battlestar final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "RSC BSTAR";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/RSC_Format_(BattleStar)";
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
