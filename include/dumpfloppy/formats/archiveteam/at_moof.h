/**
 * @file at_moof.h
 * @brief Apple Macintosh MOOF disk image.
 * @see http://fileformats.archiveteam.org/wiki/MOOF
 */
#ifndef DUMPFLOPPY_FORMATS_AT_MOOF_H
#define DUMPFLOPPY_FORMATS_AT_MOOF_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_moof final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "APPLE MOOF";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/MOOF";
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
        static constexpr uint8_t k_magic[] = {0x4du, 0x4fu, 0x4fu, 0x46u, 0xffu, 0x0au, 0x0du, 0x0au};
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
