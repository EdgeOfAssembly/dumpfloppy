/**
 * @file sk_wacky_wheels_track.h
 * @brief Wacky Wheels Track Format
 * @see https://moddingwiki.shikadi.net/wiki/Wacky_Wheels_Track_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_WACKY_WHEELS_TRACK_H
#define DUMPFLOPPY_FORMATS_SK_WACKY_WHEELS_TRACK_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_wacky_wheels_track final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "WACKY TRACK";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Wacky_Wheels_Track_Format";
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
