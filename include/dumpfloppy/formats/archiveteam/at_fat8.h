/**
 * @file at_fat8.h
 * @brief Original 8-bit FAT filesystem.
 * @see http://fileformats.archiveteam.org/wiki/FAT8
 */
#ifndef DUMPFLOPPY_FORMATS_AT_FAT8_H
#define DUMPFLOPPY_FORMATS_AT_FAT8_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_fat8 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "FAT8";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/FAT8";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] format_registry_id registry() const override
    {
        return format_registry_id::filesystem;
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
