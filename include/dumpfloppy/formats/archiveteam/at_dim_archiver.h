/**
 * @file at_dim_archiver.h
 * @brief Ray Arachelian Disk IMage Archiver (.dim).
 * @see http://fileformats.archiveteam.org/wiki/Disk_IMage_Archiver
 */
#ifndef DUMPFLOPPY_FORMATS_AT_DIM_ARCHIVER_H
#define DUMPFLOPPY_FORMATS_AT_DIM_ARCHIVER_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_dim_archiver final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DIM ARCHIVER";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/Disk_IMage_Archiver";
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
        static constexpr uint8_t k_magic[] = {0x44u, 0x69u, 0x73u, 0x6bu, 0x20u, 0x49u, 0x4du, 0x61u, 0x67u, 0x65u, 0x20u, 0x56u, 0x45u, 0x52u, 0x20u};
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
