/**
 * @file fat12.h
 * @brief FAT12 IBM PC floppy volume (raw .img / WinImage .ima).
 * @see http://fileformats.archiveteam.org/wiki/FAT
 * @see http://fileformats.archiveteam.org/wiki/Floppy_disk
 */
#ifndef DUMPFLOPPY_FORMATS_FAT12_H
#define DUMPFLOPPY_FORMATS_FAT12_H

#include "dumpfloppy/bpb.hpp"
#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

/**
 * @brief Raw FAT12 floppy image (BPB + 12-bit FAT).
 */
class fat12 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "FAT12";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/FAT";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        const std::span<const uint8_t> boot =
            data.subspan(0, data.size() < 512u ? data.size() : 512u);
        const bpb_info bpb = parse_bpb(boot);
        if (!bpb.looks_valid)
        {
            return false;
        }
        return fat_kind_from_bpb(bpb) == fat_kind::fat12;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_FORMATS_FAT12_H */
