/**
 * @file at_fdf.h
 * @brief EZ-DisKlone Plus FDF floppy image.
 * @see http://fileformats.archiveteam.org/wiki/FDF_Image
 */
#ifndef DUMPFLOPPY_FORMATS_AT_FDF_H
#define DUMPFLOPPY_FORMATS_AT_FDF_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_fdf final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "EZ DISKLONE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/FDF_Image";
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
        static constexpr uint8_t k_magic[] = {0x1au, 0x46u, 0x44u, 0x46u};
        if (data.size() >= sizeof(k_magic))
        {
            bool match = true;
            for (std::size_t i = 0; i < sizeof(k_magic); ++i)
            {
                if (data[i] != k_magic[i])
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
