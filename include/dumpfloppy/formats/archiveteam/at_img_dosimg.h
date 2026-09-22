/**
 * @file at_img_dosimg.h
 * @brief HD-Copy / DOSIMG RLE-compressed IMG floppy image.
 * @see http://fileformats.archiveteam.org/wiki/IMG_(DOSIMG)
 */
#ifndef DUMPFLOPPY_FORMATS_AT_IMG_DOSIMG_H
#define DUMPFLOPPY_FORMATS_AT_IMG_DOSIMG_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_img_dosimg final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "HD-COPY IMG";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/IMG_(DOSIMG)";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* Wiki: byte 0 is last track (39 or 79); bytes 2-166 are 0/1 track map. */
        if (data.size() < 167u)
        {
            return false;
        }
        if (data[0] != 39u && data[0] != 79u)
        {
            return false;
        }
        bool any_track = false;
        for (std::size_t i = 2; i <= 166u; ++i)
        {
            if (data[i] == 1u)
            {
                any_track = true;
            }
            else if (data[i] != 0u)
            {
                return false;
            }
        }
        return any_track;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
