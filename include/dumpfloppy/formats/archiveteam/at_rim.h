/**
 * @file at_rim.h
 * @brief EZ-DiskCopy PRO RIM floppy image.
 * @see http://fileformats.archiveteam.org/wiki/RIM
 */
#ifndef DUMPFLOPPY_FORMATS_AT_RIM_H
#define DUMPFLOPPY_FORMATS_AT_RIM_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_rim final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "EZ DISKCOPY";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/RIM";
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
        static constexpr uint8_t k_magic[] = {0x45u, 0x5au, 0x43u, 0x50u};
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
