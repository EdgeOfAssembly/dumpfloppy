/**
 * @file at_nufx.h
 * @brief Apple II NuFX / ShrinkIt archive (SDK disk images).
 * @see http://fileformats.archiveteam.org/wiki/NuFX
 */
#ifndef DUMPFLOPPY_FORMATS_AT_NUFX_H
#define DUMPFLOPPY_FORMATS_AT_NUFX_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_nufx final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "NUFX";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/NuFX";
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
        static constexpr uint8_t k_magic[] = {0x4eu, 0xf5u, 0x46u, 0xe9u, 0x6cu, 0xe5u};
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
