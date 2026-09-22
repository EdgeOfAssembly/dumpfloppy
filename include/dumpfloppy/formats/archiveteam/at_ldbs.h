/**
 * @file at_ldbs.h
 * @brief LibDsk LDBS FM/MFM floppy image.
 * @see http://fileformats.archiveteam.org/wiki/LDBS_disk_image
 */
#ifndef DUMPFLOPPY_FORMATS_AT_LDBS_H
#define DUMPFLOPPY_FORMATS_AT_LDBS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_ldbs final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "LDBS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/LDBS_disk_image";
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
