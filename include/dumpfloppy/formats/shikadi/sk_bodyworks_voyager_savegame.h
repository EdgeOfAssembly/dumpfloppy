/**
 * @file sk_bodyworks_voyager_savegame.h
 * @brief Bodyworks Voyager Savegame Format
 * @see https://moddingwiki.shikadi.net/wiki/Bodyworks_Voyager_Savegame_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_BODYWORKS_VOYAGER_SAVEGAME_H
#define DUMPFLOPPY_FORMATS_SK_BODYWORKS_VOYAGER_SAVEGAME_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_bodyworks_voyager_savegame final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "BODYWORKS SAVEGA";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Bodyworks_Voyager_Savegame_Format";
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
