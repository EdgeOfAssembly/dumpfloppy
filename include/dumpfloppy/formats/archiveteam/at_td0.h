/**
 * @file at_td0.h
 * @brief Sydex TeleDisk TD0 floppy image.
 * @see http://fileformats.archiveteam.org/wiki/TD0
 */
#ifndef DUMPFLOPPY_FORMATS_AT_TD0_H
#define DUMPFLOPPY_FORMATS_AT_TD0_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_td0 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "TELEDISK";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/TD0";
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
