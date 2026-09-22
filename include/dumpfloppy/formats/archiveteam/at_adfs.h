/**
 * @file at_adfs.h
 * @brief Acorn Advanced Disc Filing System.
 * @see http://fileformats.archiveteam.org/wiki/ADFS
 */
#ifndef DUMPFLOPPY_FORMATS_AT_ADFS_H
#define DUMPFLOPPY_FORMATS_AT_ADFS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_adfs final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "ACORN ADFS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/ADFS";
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
