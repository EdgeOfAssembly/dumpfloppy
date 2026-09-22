/**
 * @file sk_dat_god_of_thunder.h
 * @brief DAT Format (God of Thunder)
 * @see https://moddingwiki.shikadi.net/wiki/DAT_Format_(God_of_Thunder)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DAT_GOD_OF_THUNDER_H
#define DUMPFLOPPY_FORMATS_SK_DAT_GOD_OF_THUNDER_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dat_god_of_thunder final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DAT GOT";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/DAT_Format_(God_of_Thunder)";
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
