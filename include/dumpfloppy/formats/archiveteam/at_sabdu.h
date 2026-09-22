/**
 * @file at_sabdu.h
 * @brief SAB Diskette Utility floppy image.
 * @see http://fileformats.archiveteam.org/wiki/SABDU
 */
#ifndef DUMPFLOPPY_FORMATS_AT_SABDU_H
#define DUMPFLOPPY_FORMATS_AT_SABDU_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_sabdu final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SABDU";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/SABDU";
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
        static constexpr uint8_t k_magic[] = {0x53u, 0x41u, 0x42u, 0x20u, 0x44u, 0x69u, 0x73u, 0x6bu, 0x65u, 0x74u, 0x74u, 0x65u, 0x20u, 0x55u, 0x74u, 0x69u, 0x6cu, 0x69u, 0x74u, 0x79u, 0x00u};
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
