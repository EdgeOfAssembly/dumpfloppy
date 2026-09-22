/**
 * @file sk_glb_galactix.h
 * @brief GLB Format (Galactix)
 * @see https://moddingwiki.shikadi.net/wiki/GLB_Format_(Galactix)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_GLB_GALACTIX_H
#define DUMPFLOPPY_FORMATS_SK_GLB_GALACTIX_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_glb_galactix final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "GLB GALACTIX";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/GLB_Format_(Galactix)";
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
