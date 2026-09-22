/**
 * @file at_dsk_apple2.h
 * @brief Apple II raw DSK (35 tracks x 16 sectors x 256 bytes).
 * @see http://fileformats.archiveteam.org/wiki/DSK_(Apple_II)
 */
#ifndef DUMPFLOPPY_FORMATS_AT_DSK_APPLE2_H
#define DUMPFLOPPY_FORMATS_AT_DSK_APPLE2_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_dsk_apple2 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "APPLE DSK";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/DSK_(Apple_II)";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* Wiki: normal 5 1/4" image is 35*16*256 = 143360 bytes, no header. */
        return data.size() == 35u * 16u * 256u;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
