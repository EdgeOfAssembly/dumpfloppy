/**
 * @file at_cbm_1581_fs.h
 * @brief Commodore 1581 filesystem (header at 40/0).
 * @see http://fileformats.archiveteam.org/wiki/Commodore_1581_filesystem
 */
#ifndef DUMPFLOPPY_FORMATS_AT_CBM_1581_FS_H
#define DUMPFLOPPY_FORMATS_AT_CBM_1581_FS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_cbm_1581_fs final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CBM 1581 FS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/Commodore_1581_filesystem";
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
        /* Wiki: 80 tracks * 40 sectors; header 40/0; dir pointer 40/3;
         * DOS version 'D'; DOS type '3D'. 1-based track 40 -> offset 39*40*256. */
        const std::size_t hdr = 39u * 40u * 256u;
        if (data.size() < hdr + 0x1Bu)
        {
            return false;
        }
        return data[hdr] == 40u && data[hdr + 1u] == 3u
            && data[hdr + 2u] == static_cast<uint8_t>('D')
            && data[hdr + 0x19u] == static_cast<uint8_t>('3')
            && data[hdr + 0x1Au] == static_cast<uint8_t>('D');
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
