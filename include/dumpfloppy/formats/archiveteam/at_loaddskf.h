/**
 * @file at_loaddskf.h
 * @brief IBM LoadDskF / SaveDskF (DSK / SKF) floppy image.
 * @see http://fileformats.archiveteam.org/wiki/LoadDskF/SaveDskF
 */
#ifndef DUMPFLOPPY_FORMATS_AT_LOADDSKF_H
#define DUMPFLOPPY_FORMATS_AT_LOADDSKF_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_loaddskf final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "LOADDSKF";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/LoadDskF/SaveDskF";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        /* Wiki: 0xaa 0x58 (old), 0xaa 0x59 (uncompressed), 0xaa 0x5a (compressed). */
        if (data.size() < 2)
        {
            return false;
        }
        return data[0] == 0xaau
            && (data[1] == 0x58u || data[1] == 0x59u || data[1] == 0x5au);
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
