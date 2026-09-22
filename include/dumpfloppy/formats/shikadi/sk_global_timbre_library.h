/**
 * @file sk_global_timbre_library.h
 * @brief Global Timbre Library
 * @see https://moddingwiki.shikadi.net/wiki/Global_Timbre_Library
 */
#ifndef DUMPFLOPPY_FORMATS_SK_GLOBAL_TIMBRE_LIBRARY_H
#define DUMPFLOPPY_FORMATS_SK_GLOBAL_TIMBRE_LIBRARY_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_global_timbre_library final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "GTL TIMBRE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Global_Timbre_Library";
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
