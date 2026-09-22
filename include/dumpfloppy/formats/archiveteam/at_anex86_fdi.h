/**
 * @file at_anex86_fdi.h
 * @brief Anex86 PC-98 FDI floppy image.
 * @see http://fileformats.archiveteam.org/wiki/Anex86_PC98_floppy_image
 */
#ifndef DUMPFLOPPY_FORMATS_AT_ANEX86_FDI_H
#define DUMPFLOPPY_FORMATS_AT_ANEX86_FDI_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_anex86_fdi final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "PC98 FDI";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/Anex86_PC98_floppy_image";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* Wiki: 4096-byte little-endian header; headersize field at offset 8. */
        if (data.size() < 4096u)
        {
            return false;
        }
        const uint32_t header_size = static_cast<uint32_t>(data[8])
            | (static_cast<uint32_t>(data[9]) << 8)
            | (static_cast<uint32_t>(data[10]) << 16)
            | (static_cast<uint32_t>(data[11]) << 24);
        return header_size == 4096u;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
