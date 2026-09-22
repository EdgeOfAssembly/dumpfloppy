/**
 * @file at_mirage_snx.h
 * @brief Sinclair Mirage Microdrive Snapshot.
 * @see http://fileformats.archiveteam.org/wiki/Mirage_Microdrive_Snapshot
 */
#ifndef DUMPFLOPPY_FORMATS_AT_MIRAGE_SNX_H
#define DUMPFLOPPY_FORMATS_AT_MIRAGE_SNX_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_mirage_snx final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "MIRAGE SNX";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/Mirage_Microdrive_Snapshot";
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
