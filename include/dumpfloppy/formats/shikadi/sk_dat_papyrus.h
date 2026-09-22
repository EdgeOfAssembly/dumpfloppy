/**
 * @file sk_dat_papyrus.h
 * @brief DAT Format (Papyrus)
 * @see https://moddingwiki.shikadi.net/wiki/DAT_Format_(Papyrus)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DAT_PAPYRUS_H
#define DUMPFLOPPY_FORMATS_SK_DAT_PAPYRUS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dat_papyrus final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DAT PAPYRUS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/DAT_Format_(Papyrus)";
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
