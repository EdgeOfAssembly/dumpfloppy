/**
 * @file sk_dat_california_games_ii.h
 * @brief DAT Format (California Games II)
 * @see https://moddingwiki.shikadi.net/wiki/DAT_Format_(California_Games_II)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DAT_CALIFORNIA_GAMES_II_H
#define DUMPFLOPPY_FORMATS_SK_DAT_CALIFORNIA_GAMES_II_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dat_california_games_ii final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DAT CA GAMES2";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/DAT_Format_(California_Games_II)";
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
