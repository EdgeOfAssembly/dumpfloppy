/**
 * @file at_apridisk.h
 * @brief ACT Apricot ApriDisk floppy image.
 * @see http://fileformats.archiveteam.org/wiki/ApriDisk
 */
#ifndef DUMPFLOPPY_FORMATS_AT_APRIDISK_H
#define DUMPFLOPPY_FORMATS_AT_APRIDISK_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_apridisk final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "APRIDISK";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/ApriDisk";
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
        static constexpr uint8_t k_magic[] = {0x41u, 0x43u, 0x54u, 0x20u, 0x41u, 0x70u, 0x72u, 0x69u, 0x63u, 0x6fu, 0x74u, 0x20u, 0x64u, 0x69u, 0x73u, 0x6bu, 0x20u, 0x69u, 0x6du, 0x61u, 0x67u, 0x65u, 0x1au, 0x04u};
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
