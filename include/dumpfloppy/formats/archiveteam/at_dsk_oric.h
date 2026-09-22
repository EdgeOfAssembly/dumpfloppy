/**
 * @file at_dsk_oric.h
 * @brief Oric DSK disk image (ORICDISK / MFM_DISK).
 * @see http://fileformats.archiveteam.org/wiki/DSK_(Oric)
 */
#ifndef DUMPFLOPPY_FORMATS_AT_DSK_ORIC_H
#define DUMPFLOPPY_FORMATS_AT_DSK_ORIC_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_dsk_oric final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "ORIC DSK";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/DSK_(Oric)";
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
        static constexpr uint8_t k_magic_0[] = {0x4fu, 0x52u, 0x49u, 0x43u, 0x44u, 0x49u, 0x53u, 0x4bu};
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
        static constexpr uint8_t k_magic_1[] = {0x4du, 0x46u, 0x4du, 0x5fu, 0x44u, 0x49u, 0x53u, 0x4bu};
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
