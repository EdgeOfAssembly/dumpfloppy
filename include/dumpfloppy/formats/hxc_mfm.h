/**
 * @file hxc_mfm.h
 * @brief HxC Floppy Emulator raw MFM bitstream image (`.mfm`).
 * @see http://fileformats.archiveteam.org/wiki/HxC_Floppy_Emulator
 */
#ifndef DUMPFLOPPY_FORMATS_HXC_MFM_H
#define DUMPFLOPPY_FORMATS_HXC_MFM_H

#include "dumpfloppy/format.h"

#include <cstring>

namespace dumpfloppy
{
namespace formats
{

class hxc_mfm final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "HXC MFM";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/HxC_Floppy_Emulator";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        return data.size() >= 6u && std::memcmp(data.data(), "HXCMFM", 6) == 0;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
