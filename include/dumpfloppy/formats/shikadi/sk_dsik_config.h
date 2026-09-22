/**
 * @file sk_dsik_config.h
 * @brief DSIK Config Format
 * @see https://moddingwiki.shikadi.net/wiki/DSIK_Config_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DSIK_CONFIG_H
#define DUMPFLOPPY_FORMATS_SK_DSIK_CONFIG_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dsik_config final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DSIK CONFIG";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/DSIK_Config_Format";
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
