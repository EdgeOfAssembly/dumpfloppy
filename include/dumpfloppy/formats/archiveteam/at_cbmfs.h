/**
 * @file at_cbmfs.h
 * @brief Commodore CBMFS (1541 BAM at track 18 sector 0).
 * @see http://fileformats.archiveteam.org/wiki/CBMFS
 */
#ifndef DUMPFLOPPY_FORMATS_AT_CBMFS_H
#define DUMPFLOPPY_FORMATS_AT_CBMFS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_cbmfs final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CBMFS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/CBMFS";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* Wiki: BAM at 18/0; DOS version $41 ('A') or $00; DOS type usually "2A".
         * 1541 article: tracks 1-17 are the outer 21-sector zone (17*21*256). */
        const std::size_t bam = 17u * 21u * 256u;
        if (data.size() < bam + 0xA7u)
        {
            return false;
        }
        if (data[bam] != 18u || data[bam + 1u] != 1u)
        {
            return false;
        }
        if (data[bam + 2u] != static_cast<uint8_t>('A') && data[bam + 2u] != 0u)
        {
            return false;
        }
        return data[bam + 0xA5u] == static_cast<uint8_t>('2')
            && data[bam + 0xA6u] == static_cast<uint8_t>('A');
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
