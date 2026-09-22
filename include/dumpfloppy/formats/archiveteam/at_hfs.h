/**
 * @file at_hfs.h
 * @brief Apple Hierarchical File System (Mac OS Standard).
 * @see http://fileformats.archiveteam.org/wiki/HFS
 */
#ifndef DUMPFLOPPY_FORMATS_AT_HFS_H
#define DUMPFLOPPY_FORMATS_AT_HFS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_hfs final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "APPLE HFS";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/HFS";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* Wiki: ASCII "BD" at offset 1024. */
        if (data.size() < 1026u)
        {
            return false;
        }
        return data[1024] == static_cast<uint8_t>('B')
            && data[1025] == static_cast<uint8_t>('D');
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
