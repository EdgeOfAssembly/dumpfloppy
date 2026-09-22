/**
 * @file sk_miscellaneous_wacky_wheels_file.h
 * @brief Miscellaneous Wacky Wheels File Formats
 * @see https://moddingwiki.shikadi.net/wiki/Miscellaneous_Wacky_Wheels_File_Formats
 */
#ifndef DUMPFLOPPY_FORMATS_SK_MISCELLANEOUS_WACKY_WHEELS_FILE_H
#define DUMPFLOPPY_FORMATS_SK_MISCELLANEOUS_WACKY_WHEELS_FILE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_miscellaneous_wacky_wheels_file final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "WACKY MISC";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Miscellaneous_Wacky_Wheels_File_Formats";
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
