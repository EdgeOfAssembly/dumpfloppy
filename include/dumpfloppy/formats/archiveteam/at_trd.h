/**
 * @file at_trd.h
 * @brief ZX Spectrum TR-DOS TRD sector dump.
 * @see http://fileformats.archiveteam.org/wiki/TRD
 */
#ifndef DUMPFLOPPY_FORMATS_AT_TRD_H
#define DUMPFLOPPY_FORMATS_AT_TRD_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_trd final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "TR-DOS TRD";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/TRD";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* Wiki (TR-DOS filesystem): 256-byte sectors, 16/track; sizes 655360,
         * 327680, or 163840. TRD page notes files may be shorter if unused
         * sectors are omitted — only full sizes are sniffed here. */
        return data.size() == 655360u || data.size() == 327680u
            || data.size() == 163840u;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
