/**
 * @file at_ima.h
 * @brief WinImage-style raw floppy dump (.ima / .img).
 * @see http://fileformats.archiveteam.org/wiki/IMA
 */
#ifndef DUMPFLOPPY_FORMATS_AT_IMA_H
#define DUMPFLOPPY_FORMATS_AT_IMA_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_ima final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "PC IMA";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/IMA";
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
