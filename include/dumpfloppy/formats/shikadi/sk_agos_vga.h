/**
 * @file sk_agos_vga.h
 * @brief AGOS VGA Format
 * @see https://moddingwiki.shikadi.net/wiki/AGOS_VGA_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_AGOS_VGA_H
#define DUMPFLOPPY_FORMATS_SK_AGOS_VGA_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_agos_vga final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "AGOS VGA";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/AGOS_VGA_Format";
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
