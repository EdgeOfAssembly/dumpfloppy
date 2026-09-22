/**
 * @file sk_death_rally_haf.h
 * @brief Death Rally HAF Format
 * @see https://moddingwiki.shikadi.net/wiki/Death_Rally_HAF_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DEATH_RALLY_HAF_H
#define DUMPFLOPPY_FORMATS_SK_DEATH_RALLY_HAF_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_death_rally_haf final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DEATH RALLY HAF";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Death_Rally_HAF_Format";
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
