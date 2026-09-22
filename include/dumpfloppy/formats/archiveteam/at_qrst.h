/**
 * @file at_qrst.h
 * @brief Compaq Quick Release Sector Transfer floppy image.
 * @see http://fileformats.archiveteam.org/wiki/Quick_Release_Sector_Transfer
 */
#ifndef DUMPFLOPPY_FORMATS_AT_QRST_H
#define DUMPFLOPPY_FORMATS_AT_QRST_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_qrst final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "COMPAQ QRST";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/Quick_Release_Sector_Transfer";
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
        static constexpr uint8_t k_magic[] = {0x51u, 0x52u, 0x53u, 0x54u};
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
