/**
 * @file sk_catacomb_savegame.h
 * @brief Catacomb Savegame Format
 * @see https://moddingwiki.shikadi.net/wiki/Catacomb_Savegame_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CATACOMB_SAVEGAME_H
#define DUMPFLOPPY_FORMATS_SK_CATACOMB_SAVEGAME_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_catacomb_savegame final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CATACOMB SAVEGAM";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Catacomb_Savegame_Format";
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
