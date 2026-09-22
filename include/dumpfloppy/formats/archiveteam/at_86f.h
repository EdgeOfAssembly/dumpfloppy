/**
 * @file at_86f.h
 * @brief 86Box 86F floppy image (86BF / 86bf).
 * @see http://fileformats.archiveteam.org/wiki/86F
 */
#ifndef DUMPFLOPPY_FORMATS_AT_86F_H
#define DUMPFLOPPY_FORMATS_AT_86F_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_86f final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "86BOX 86F";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/86F";
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
        static constexpr uint8_t k_magic_0[] = {0x38u, 0x36u, 0x42u, 0x46u};
        if (data.size() >= sizeof(k_magic_0))
        {
            bool match = true;
            for (std::size_t i = 0; i < sizeof(k_magic_0); ++i)
            {
                if (data[i] != k_magic_0[i])
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
        static constexpr uint8_t k_magic_1[] = {0x38u, 0x36u, 0x62u, 0x66u};
        if (data.size() >= sizeof(k_magic_1))
        {
            bool match = true;
            for (std::size_t i = 0; i < sizeof(k_magic_1); ++i)
            {
                if (data[i] != k_magic_1[i])
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
