/**
 * @file at_kryoflux.h
 * @brief KryoFlux stream dump (.raw).
 * @see http://fileformats.archiveteam.org/wiki/KryoFlux
 */
#ifndef DUMPFLOPPY_FORMATS_AT_KRYOFLUX_H
#define DUMPFLOPPY_FORMATS_AT_KRYOFLUX_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_kryoflux final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "KRYOFLUX";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/KryoFlux";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* Wiki: likely begins with 0d 04 (OOB KFInfo); ASCII
         * "KryoFlux DiskSystem" often at offset 54. */
        if (data.size() < 73u)
        {
            return false;
        }
        if (data[0] != 0x0du || data[1] != 0x04u)
        {
            return false;
        }
        static constexpr uint8_t k_id[] = {
            0x4b, 0x72, 0x79, 0x6f, 0x46, 0x6c, 0x75, 0x78, 0x20,
            0x44, 0x69, 0x73, 0x6b, 0x53, 0x79, 0x73, 0x74, 0x65, 0x6d};
        for (std::size_t i = 0; i < sizeof(k_id); ++i)
        {
            if (data[54u + i] != k_id[i])
            {
                return false;
            }
        }
        return true;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
