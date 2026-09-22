/**
 * @file sk_rol_image_archive.h
 * @brief ROL Format (image archive)
 * @see https://moddingwiki.shikadi.net/wiki/ROL_Format_(image_archive)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_ROL_IMAGE_ARCHIVE_H
#define DUMPFLOPPY_FORMATS_SK_ROL_IMAGE_ARCHIVE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_rol_image_archive final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "ROL IMAGE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/ROL_Format_(image_archive)";
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
