/**
 * @file sk_kort_sound_archive.h
 * @brief KORT Sound Archive
 * @see https://moddingwiki.shikadi.net/wiki/KORT_Sound_Archive
 */
#ifndef DUMPFLOPPY_FORMATS_SK_KORT_SOUND_ARCHIVE_H
#define DUMPFLOPPY_FORMATS_SK_KORT_SOUND_ARCHIVE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_kort_sound_archive final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "KORT ARCHIVE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/KORT_Sound_Archive";
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
