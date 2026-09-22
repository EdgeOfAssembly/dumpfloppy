/**
 * @file sk_oregon_trail_tombstones.h
 * @brief Oregon Trail tombstones format
 * @see https://moddingwiki.shikadi.net/wiki/Oregon_Trail_tombstones_format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_OREGON_TRAIL_TOMBSTONES_H
#define DUMPFLOPPY_FORMATS_SK_OREGON_TRAIL_TOMBSTONES_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_oregon_trail_tombstones final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "OREGON TOMBSTONE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Oregon_Trail_tombstones_format";
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
