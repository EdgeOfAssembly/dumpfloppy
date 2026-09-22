/**
 * @file at_dart.h
 * @brief Apple DART compressed Lisa / early Mac disk image.
 * @see http://fileformats.archiveteam.org/wiki/DART
 */
#ifndef DUMPFLOPPY_FORMATS_AT_DART_H
#define DUMPFLOPPY_FORMATS_AT_DART_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_dart final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "APPLE DART";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/DART";
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
