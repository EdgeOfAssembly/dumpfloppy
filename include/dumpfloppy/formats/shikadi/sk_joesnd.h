/**
 * @file sk_joesnd.h
 * @brief JOESND
 * @see https://moddingwiki.shikadi.net/wiki/JOESND
 */
#ifndef DUMPFLOPPY_FORMATS_SK_JOESND_H
#define DUMPFLOPPY_FORMATS_SK_JOESND_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_joesnd final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "JOESND";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/JOESND";
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
