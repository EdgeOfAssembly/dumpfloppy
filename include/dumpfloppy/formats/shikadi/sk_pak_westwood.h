/**
 * @file sk_pak_westwood.h
 * @brief PAK Format (Westwood)
 * @see https://moddingwiki.shikadi.net/wiki/PAK_Format_(Westwood)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_PAK_WESTWOOD_H
#define DUMPFLOPPY_FORMATS_SK_PAK_WESTWOOD_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_pak_westwood final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "PAK WW";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/PAK_Format_(Westwood)";
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
