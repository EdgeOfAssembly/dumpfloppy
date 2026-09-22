/**
 * @file sk_death_rally_cmf.h
 * @brief Death Rally CMF Format
 * @see https://moddingwiki.shikadi.net/wiki/Death_Rally_CMF_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DEATH_RALLY_CMF_H
#define DUMPFLOPPY_FORMATS_SK_DEATH_RALLY_CMF_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_death_rally_cmf final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DEATH RALLY CMF";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Death_Rally_CMF_Format";
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
