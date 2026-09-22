/**
 * @file sk_command_and_conquer_red_alert_vortex_lookup_table.h
 * @brief Command & Conquer: Red Alert Vortex Lookup Table
 * @see https://moddingwiki.shikadi.net/wiki/Command_&_Conquer:_Red_Alert_Vortex_Lookup_Table
 */
#ifndef DUMPFLOPPY_FORMATS_SK_COMMAND_AND_CONQUER_RED_ALERT_VORTEX_LOOKUP_TABLE_H
#define DUMPFLOPPY_FORMATS_SK_COMMAND_AND_CONQUER_RED_ALERT_VORTEX_LOOKUP_TABLE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_command_and_conquer_red_alert_vortex_lookup_table final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "RA VORTEX LUT";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Command_&_Conquer:_Red_Alert_Vortex_Lookup_Table";
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
