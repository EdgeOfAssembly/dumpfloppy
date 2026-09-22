/**
 * @file at_st.h
 * @brief Atari ST raw .ST floppy image.
 * @see http://fileformats.archiveteam.org/wiki/ST_disk_image
 */
#ifndef DUMPFLOPPY_FORMATS_AT_ST_H
#define DUMPFLOPPY_FORMATS_AT_ST_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_st final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "ATARI ST";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/ST_disk_image";
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
