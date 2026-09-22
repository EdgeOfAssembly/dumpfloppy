/**
 * @file at_jam_packer.h
 * @brief Atari ST JAM Packer disk archive / compressor.
 * @see http://fileformats.archiveteam.org/wiki/The_JAM_Packer
 */
#ifndef DUMPFLOPPY_FORMATS_AT_JAM_PACKER_H
#define DUMPFLOPPY_FORMATS_AT_JAM_PACKER_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_jam_packer final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "JAM PACKER";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/The_JAM_Packer";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        (void)data;
        return false;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
