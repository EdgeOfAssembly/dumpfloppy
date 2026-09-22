/**
 * @file sk_res_stellar_7.h
 * @brief RES Format (Stellar 7)
 * @see https://moddingwiki.shikadi.net/wiki/RES_Format_(Stellar_7)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_RES_STELLAR_7_H
#define DUMPFLOPPY_FORMATS_SK_RES_STELLAR_7_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_res_stellar_7 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "RES STELLAR7";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/RES_Format_(Stellar_7)";
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
