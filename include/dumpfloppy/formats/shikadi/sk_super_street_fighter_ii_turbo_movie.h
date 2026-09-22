/**
 * @file sk_super_street_fighter_ii_turbo_movie.h
 * @brief Super Street Fighter II TURBO Movie Format
 * @see https://moddingwiki.shikadi.net/wiki/Super_Street_Fighter_II_TURBO_Movie_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_SUPER_STREET_FIGHTER_II_TURBO_MOVIE_H
#define DUMPFLOPPY_FORMATS_SK_SUPER_STREET_FIGHTER_II_TURBO_MOVIE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_super_street_fighter_ii_turbo_movie final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SSF2 MOVIE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Super_Street_Fighter_II_TURBO_Movie_Format";
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
