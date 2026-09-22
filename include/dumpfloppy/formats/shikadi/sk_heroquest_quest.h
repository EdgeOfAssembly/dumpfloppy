/**
 * @file sk_heroquest_quest.h
 * @brief HeroQuest Quest Format
 * @see https://moddingwiki.shikadi.net/wiki/HeroQuest_Quest_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_HEROQUEST_QUEST_H
#define DUMPFLOPPY_FORMATS_SK_HEROQUEST_QUEST_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_heroquest_quest final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "HEROQUEST QUEST";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/HeroQuest_Quest_Format";
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
