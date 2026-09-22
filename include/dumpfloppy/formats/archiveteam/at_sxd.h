/**
 * @file at_sxd.h
 * @brief Sydex CopyQM Plus SXD self-extracting floppy image.
 * @see http://fileformats.archiveteam.org/wiki/SXD_(Sydex)
 */
#ifndef DUMPFLOPPY_FORMATS_AT_SXD_H
#define DUMPFLOPPY_FORMATS_AT_SXD_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_sxd final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "COPYQM PLUS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/SXD_(Sydex)";
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
        static constexpr uint8_t k_magic[] = {0x53u, 0x58u, 0x44u};
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
