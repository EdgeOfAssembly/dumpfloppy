/**
 * @file at_disk_imploder.h
 * @brief Amiga Disk Imploder compressed disk image (.dmp / .dex).
 * @see http://fileformats.archiveteam.org/wiki/Disk_Imploder
 */
#ifndef DUMPFLOPPY_FORMATS_AT_DISK_IMPLODER_H
#define DUMPFLOPPY_FORMATS_AT_DISK_IMPLODER_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_disk_imploder final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "AMIGA DMP";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/Disk_Imploder";
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
