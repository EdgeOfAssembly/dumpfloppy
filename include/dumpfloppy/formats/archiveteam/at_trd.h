/**
 * @file at_trd.h
 * @brief ZX Spectrum TR-DOS TRD sector dump.
 * @see http://fileformats.archiveteam.org/wiki/TRD
 */
#ifndef DUMPFLOPPY_FORMATS_AT_TRD_H
#define DUMPFLOPPY_FORMATS_AT_TRD_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_trd final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "TR-DOS TRD";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/TRD";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* Wiki (TR-DOS filesystem): 256-byte sectors, 16/track; full sizes
         * 655360 / 327680 / 163840. Those last two collide with IBM 320K/160K
         * .img, so size alone is not enough — require the disk-info block at
         * logical sector 8 (ID 0xE7 == 0x10, or 0xE1–0xE4 geometry). */
        const std::size_t n = data.size();
        if (n != 655360u && n != 327680u && n != 163840u)
        {
            return false;
        }
        constexpr std::size_t k_info = 8u * 256u;
        if (n < k_info + 0xE8u)
        {
            return false;
        }
        const uint8_t id = data[k_info + 0xE7u];
        if (id == 0x10u)
        {
            return true;
        }
        const uint8_t first_sec = data[k_info + 0xE1u];
        const uint8_t disk_type = data[k_info + 0xE3u];
        return first_sec < 16u && disk_type >= 0x16u && disk_type <= 0x19u;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
