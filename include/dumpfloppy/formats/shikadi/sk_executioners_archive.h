/**
 * @file sk_executioners_archive.h
 * @brief Executioners Archive Format
 * @see https://moddingwiki.shikadi.net/wiki/Executioners_Archive_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_EXECUTIONERS_ARCHIVE_H
#define DUMPFLOPPY_FORMATS_SK_EXECUTIONERS_ARCHIVE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_executioners_archive final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "EXECUTIONERS ARC";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Executioners_Archive_Format";
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
