/**
 * @file at_wrp.h
 * @brief Amiga WRP (Warp) disk image.
 * @see http://fileformats.archiveteam.org/wiki/WRP
 */
#ifndef DUMPFLOPPY_FORMATS_AT_WRP_H
#define DUMPFLOPPY_FORMATS_AT_WRP_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_wrp final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "AMIGA WRP";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/WRP";
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
        static constexpr uint8_t k_magic[] = {0x57u, 0x61u, 0x72u, 0x70u, 0x20u, 0x76u};
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
