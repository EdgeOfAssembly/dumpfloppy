/**
 * @file sk_pic_sexonix.h
 * @brief PIC Format (SeXoniX)
 * @see https://moddingwiki.shikadi.net/wiki/PIC_Format_(SeXoniX)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_PIC_SEXONIX_H
#define DUMPFLOPPY_FORMATS_SK_PIC_SEXONIX_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_pic_sexonix final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "PIC SEXONIX";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/PIC_Format_(SeXoniX)";
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
