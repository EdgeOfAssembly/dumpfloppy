/**
 * @file sk_jazz_jackrabbit_cutscene.h
 * @brief Jazz Jackrabbit Cutscene Format
 * @see https://moddingwiki.shikadi.net/wiki/Jazz_Jackrabbit_Cutscene_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_JAZZ_JACKRABBIT_CUTSCENE_H
#define DUMPFLOPPY_FORMATS_SK_JAZZ_JACKRABBIT_CUTSCENE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_jazz_jackrabbit_cutscene final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "JAZZ CUTSCENE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Jazz_Jackrabbit_Cutscene_Format";
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
