/**
 * @file sk_dat_zool.h
 * @brief DAT Format (Zool)
 * @see https://moddingwiki.shikadi.net/wiki/DAT_Format_(Zool)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DAT_ZOOL_H
#define DUMPFLOPPY_FORMATS_SK_DAT_ZOOL_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dat_zool final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DAT ZOOL";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/DAT_Format_(Zool)";
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
