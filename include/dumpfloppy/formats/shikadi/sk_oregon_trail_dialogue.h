/**
 * @file sk_oregon_trail_dialogue.h
 * @brief Oregon Trail dialogue format
 * @see https://moddingwiki.shikadi.net/wiki/Oregon_Trail_dialogue_format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_OREGON_TRAIL_DIALOGUE_H
#define DUMPFLOPPY_FORMATS_SK_OREGON_TRAIL_DIALOGUE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_oregon_trail_dialogue final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "OREGON DIALOGUE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Oregon_Trail_dialogue_format";
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
