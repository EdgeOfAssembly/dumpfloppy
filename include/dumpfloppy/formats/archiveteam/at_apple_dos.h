/**
 * @file at_apple_dos.h
 * @brief Apple DOS 3.x filesystem (VTOC at track $11 sector $00).
 * @see http://fileformats.archiveteam.org/wiki/Apple_DOS_file_system
 */
#ifndef DUMPFLOPPY_FORMATS_AT_APPLE_DOS_H
#define DUMPFLOPPY_FORMATS_AT_APPLE_DOS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_apple_dos final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "APPLE DOS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/Apple_DOS_file_system";
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
        /* Wiki: VTOC at track $11 sector $00; tracks normally 35 (up to 40);
         * sectors/track 13 or 16; bytes/sector 256. */
        const auto vtoc_ok = [](std::span<const uint8_t> img, std::size_t off) -> bool
        {
            if (img.size() < off + 0x38u)
            {
                return false;
            }
            const uint8_t tracks = img[off + 0x34u];
            const uint8_t spt = img[off + 0x35u];
            const unsigned bps = static_cast<unsigned>(img[off + 0x36u])
                | (static_cast<unsigned>(img[off + 0x37u]) << 8);
            return tracks >= 35u && tracks <= 40u && (spt == 13u || spt == 16u)
                && bps == 256u;
        };
        return vtoc_ok(data, 17u * 16u * 256u) || vtoc_ok(data, 17u * 13u * 256u);
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
