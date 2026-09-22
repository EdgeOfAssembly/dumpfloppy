/**
 * @file at_atari_fms.h
 * @brief Atari File Management Subsystem (Atari 810).
 * @see http://fileformats.archiveteam.org/wiki/Atari_File_Management_Subsystem
 */
#ifndef DUMPFLOPPY_FORMATS_AT_ATARI_FMS_H
#define DUMPFLOPPY_FORMATS_AT_ATARI_FMS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_atari_fms final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "ATARI FMS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/Atari_File_Management_Subsystem";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* Wiki boot record: byte 0 = 0, byte 1 = 1, byte 6 = 0x4B (JMP). */
        if (data.size() < 9u)
        {
            return false;
        }
        return data[0] == 0u && data[1] == 1u && data[6] == 0x4Bu;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
