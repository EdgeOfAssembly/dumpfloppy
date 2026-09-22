/**
 * @file at_prodos.h
 * @brief Apple ProDOS filesystem.
 * @see http://fileformats.archiveteam.org/wiki/ProDOS_file_system
 */
#ifndef DUMPFLOPPY_FORMATS_AT_PRODOS_H
#define DUMPFLOPPY_FORMATS_AT_PRODOS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_prodos final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "APPLE PRODOS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/ProDOS_file_system";
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
