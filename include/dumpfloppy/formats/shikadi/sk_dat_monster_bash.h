/**
 * @file sk_dat_monster_bash.h
 * @brief DAT Format (Monster Bash)
 * @see https://moddingwiki.shikadi.net/wiki/DAT_Format_(Monster_Bash)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DAT_MONSTER_BASH_H
#define DUMPFLOPPY_FORMATS_SK_DAT_MONSTER_BASH_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dat_monster_bash final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DAT MBASH";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/DAT_Format_(Monster_Bash)";
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
