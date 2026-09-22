/**
 * @file at_cp2.h
 * @brief Copy-II-PC / Snatch-It CP2 disk image.
 * @see http://fileformats.archiveteam.org/wiki/CP2
 */
#ifndef DUMPFLOPPY_FORMATS_AT_CP2_H
#define DUMPFLOPPY_FORMATS_AT_CP2_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_cp2 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "COPY II PC";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/CP2";
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
        static constexpr uint8_t k_magic[] = {0x53u, 0x4fu, 0x46u, 0x54u, 0x57u, 0x41u, 0x52u, 0x45u, 0x20u, 0x50u, 0x49u, 0x52u, 0x41u, 0x54u, 0x45u, 0x53u, 0x52u, 0x65u, 0x6cu, 0x65u, 0x61u, 0x73u, 0x65u, 0x20u};
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
