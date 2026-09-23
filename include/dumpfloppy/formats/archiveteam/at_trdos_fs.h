/**
 * @file at_trdos_fs.h
 * @brief ZX Spectrum TR-DOS filesystem.
 * @see http://fileformats.archiveteam.org/wiki/TR-DOS_filesystem
 */
#ifndef DUMPFLOPPY_FORMATS_AT_TRDOS_FS_H
#define DUMPFLOPPY_FORMATS_AT_TRDOS_FS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_trdos_fs final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "TR-DOS FS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/TR-DOS_filesystem";
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
