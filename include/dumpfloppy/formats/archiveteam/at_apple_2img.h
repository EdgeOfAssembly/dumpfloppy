/**
 * @file at_apple_2img.h
 * @brief Apple Disk Image with 2IMG prefix (.2mg / related).
 * @see http://fileformats.archiveteam.org/wiki/Apple_Disk_Image
 */
#ifndef DUMPFLOPPY_FORMATS_AT_APPLE_2IMG_H
#define DUMPFLOPPY_FORMATS_AT_APPLE_2IMG_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_apple_2img final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "APPLE 2IMG";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/Apple_Disk_Image";
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
        static constexpr uint8_t k_magic[] = {0x32u, 0x49u, 0x4du, 0x47u};
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
