/**
 * @file at_dmk.h
 * @brief Tandy / TRS-80 DMK disk image.
 * @see http://fileformats.archiveteam.org/wiki/DMK
 */
#ifndef DUMPFLOPPY_FORMATS_AT_DMK_H
#define DUMPFLOPPY_FORMATS_AT_DMK_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_dmk final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "TANDY DMK";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/DMK";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
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
