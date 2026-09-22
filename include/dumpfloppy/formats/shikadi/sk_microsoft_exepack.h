/**
 * @file sk_microsoft_exepack.h
 * @brief Microsoft EXEPACK
 * @see https://moddingwiki.shikadi.net/wiki/Microsoft_EXEPACK
 */
#ifndef DUMPFLOPPY_FORMATS_SK_MICROSOFT_EXEPACK_H
#define DUMPFLOPPY_FORMATS_SK_MICROSOFT_EXEPACK_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_microsoft_exepack final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "EXEPACK";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Microsoft_EXEPACK";
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
