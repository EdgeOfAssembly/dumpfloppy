/**
 * @file at_cpm.h
 * @brief CP/M filesystem on floppy (not self-describing).
 * @see http://fileformats.archiveteam.org/wiki/CP/M_file_system
 */
#ifndef DUMPFLOPPY_FORMATS_AT_CPM_H
#define DUMPFLOPPY_FORMATS_AT_CPM_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_cpm final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CP/M FS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/CP/M_file_system";
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
