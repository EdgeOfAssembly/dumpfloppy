/**
 * @file sk_smacker_video.h
 * @brief Smacker Video Format
 * @see https://moddingwiki.shikadi.net/wiki/Smacker_Video_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_SMACKER_VIDEO_H
#define DUMPFLOPPY_FORMATS_SK_SMACKER_VIDEO_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_smacker_video final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SMACKER";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Smacker_Video_Format";
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
