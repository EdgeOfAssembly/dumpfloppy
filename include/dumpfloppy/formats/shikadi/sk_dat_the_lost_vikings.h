/**
 * @file sk_dat_the_lost_vikings.h
 * @brief DAT Format (The Lost Vikings)
 * @see https://moddingwiki.shikadi.net/wiki/DAT_Format_(The_Lost_Vikings)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DAT_THE_LOST_VIKINGS_H
#define DUMPFLOPPY_FORMATS_SK_DAT_THE_LOST_VIKINGS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dat_the_lost_vikings final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DAT VIKINGS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/DAT_Format_(The_Lost_Vikings)";
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
