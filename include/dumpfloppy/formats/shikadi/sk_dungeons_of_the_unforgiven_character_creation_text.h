/**
 * @file sk_dungeons_of_the_unforgiven_character_creation_text.h
 * @brief Dungeons of the Unforgiven Character Creation Text
 * @see https://moddingwiki.shikadi.net/wiki/Dungeons_of_the_Unforgiven_Character_Creation_Text
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DUNGEONS_OF_THE_UNFORGIVEN_CHARACTER_CREATION_TEXT_H
#define DUMPFLOPPY_FORMATS_SK_DUNGEONS_OF_THE_UNFORGIVEN_CHARACTER_CREATION_TEXT_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dungeons_of_the_unforgiven_character_creation_text final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DOTU CHAR TXT";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Dungeons_of_the_Unforgiven_Character_Creation_Text";
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
