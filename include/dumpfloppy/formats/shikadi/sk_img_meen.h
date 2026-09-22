/**
 * @file sk_img_meen.h
 * @brief IMG Format (Meen)
 * @see https://moddingwiki.shikadi.net/wiki/IMG_Format_(Meen)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_IMG_MEEN_H
#define DUMPFLOPPY_FORMATS_SK_IMG_MEEN_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_img_meen final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "IMG MEEN";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/IMG_Format_(Meen)";
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
