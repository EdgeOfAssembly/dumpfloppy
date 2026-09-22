/**
 * @file at_apd.h
 * @brief APF Imagination Machine emulated disk (.apd).
 * @see http://fileformats.archiveteam.org/wiki/APD
 */
#ifndef DUMPFLOPPY_FORMATS_AT_APD_H
#define DUMPFLOPPY_FORMATS_AT_APD_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_apd final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "APF APD";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/APD";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* Wiki: supposed to be 68K, though some are 64K. */
        return data.size() == 68u * 1024u || data.size() == 64u * 1024u;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
