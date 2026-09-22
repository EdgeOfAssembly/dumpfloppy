/**
 * @file sk_dat_bullfrog.h
 * @brief DAT Format (Bullfrog)
 * @see https://moddingwiki.shikadi.net/wiki/DAT_Format_(Bullfrog)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DAT_BULLFROG_H
#define DUMPFLOPPY_FORMATS_SK_DAT_BULLFROG_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dat_bullfrog final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DAT BULLFROG";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/DAT_Format_(Bullfrog)";
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
