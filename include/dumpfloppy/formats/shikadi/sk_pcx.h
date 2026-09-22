/**
 * @file sk_pcx.h
 * @brief PCX Format
 * @see https://moddingwiki.shikadi.net/wiki/PCX_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_PCX_H
#define DUMPFLOPPY_FORMATS_SK_PCX_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_pcx final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "PCX";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/PCX_Format";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        if (data.size() < 4)
        {
            return false;
        }
        if (data[0] != 0x0A)
        {
            return false;
        }
        const uint8_t ver = data[1];
        if (!(ver == 0 || ver == 2 || ver == 3 || ver == 4 || ver == 5))
        {
            return false;
        }
        const uint8_t enc = data[2];
        if (!(enc == 0 || enc == 1))
        {
            return false;
        }
        const uint8_t bpp = data[3];
        return bpp == 1 || bpp == 2 || bpp == 4 || bpp == 8 || bpp == 24;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
