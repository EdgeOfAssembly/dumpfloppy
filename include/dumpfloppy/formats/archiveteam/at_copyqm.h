/**
 * @file at_copyqm.h
 * @brief Sydex CopyQM floppy disk image.
 * @see http://fileformats.archiveteam.org/wiki/CopyQM
 */
#ifndef DUMPFLOPPY_FORMATS_AT_COPYQM_H
#define DUMPFLOPPY_FORMATS_AT_COPYQM_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_copyqm final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "COPYQM";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/CopyQM";
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
