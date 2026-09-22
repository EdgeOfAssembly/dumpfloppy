/**
 * @file at_zoom.h
 * @brief Amiga Zoom disk image (ZOOM / ZOM5).
 * @see http://fileformats.archiveteam.org/wiki/Zoom_disk_image
 */
#ifndef DUMPFLOPPY_FORMATS_AT_ZOOM_H
#define DUMPFLOPPY_FORMATS_AT_ZOOM_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_zoom final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "AMIGA ZOOM";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/Zoom_disk_image";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        if (data.empty())
        {
            return false;
        }
        static constexpr uint8_t k_magic_0[] = {0x5au, 0x4fu, 0x4fu, 0x4du};
        if (data.size() >= sizeof(k_magic_0))
        {
            bool match = true;
            for (std::size_t i = 0; i < sizeof(k_magic_0); ++i)
            {
                if (data[i] != k_magic_0[i])
                {
                    match = false;
                    break;
                }
            }
            if (match)
            {
                return true;
            }
        }
        static constexpr uint8_t k_magic_1[] = {0x5au, 0x4fu, 0x4du, 0x35u};
        if (data.size() >= sizeof(k_magic_1))
        {
            bool match = true;
            for (std::size_t i = 0; i < sizeof(k_magic_1); ++i)
            {
                if (data[i] != k_magic_1[i])
                {
                    match = false;
                    break;
                }
            }
            if (match)
            {
                return true;
            }
        }
        return false;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
