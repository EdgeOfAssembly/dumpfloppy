/**
 * @file at_edd.h
 * @brief Apple II Essential Data Duplicator (EDD) raw image.
 * @see http://fileformats.archiveteam.org/wiki/EDD_disk_image
 */
#ifndef DUMPFLOPPY_FORMATS_AT_EDD_H
#define DUMPFLOPPY_FORMATS_AT_EDD_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_edd final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "APPLE EDD";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/EDD_disk_image";
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
