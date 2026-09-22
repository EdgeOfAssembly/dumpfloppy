/**
 * @file sk_duke_nukem_ii_actor_info.h
 * @brief Duke Nukem II Actor Info
 * @see https://moddingwiki.shikadi.net/wiki/Duke_Nukem_II_Actor_Info
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DUKE_NUKEM_II_ACTOR_INFO_H
#define DUMPFLOPPY_FORMATS_SK_DUKE_NUKEM_II_ACTOR_INFO_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_duke_nukem_ii_actor_info final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DUKE INFO";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Duke_Nukem_II_Actor_Info";
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
