/**
 * @file sk_treasure_mountain_save_file.h
 * @brief Treasure Mountain Save File
 * @see https://moddingwiki.shikadi.net/wiki/Treasure_Mountain_Save_File
 */
#ifndef DUMPFLOPPY_FORMATS_SK_TREASURE_MOUNTAIN_SAVE_FILE_H
#define DUMPFLOPPY_FORMATS_SK_TREASURE_MOUNTAIN_SAVE_FILE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_treasure_mountain_save_file final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "TREASURE FILE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Treasure_Mountain_Save_File";
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
