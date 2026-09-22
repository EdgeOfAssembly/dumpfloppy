/**
 * @file sk_all_dogs_archive.h
 * @brief All Dogs Archive Format
 * @see https://moddingwiki.shikadi.net/wiki/All_Dogs_Archive_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_ALL_DOGS_ARCHIVE_H
#define DUMPFLOPPY_FORMATS_SK_ALL_DOGS_ARCHIVE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_all_dogs_archive final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "ALL DOGS ARCHIVE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/All_Dogs_Archive_Format";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
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
