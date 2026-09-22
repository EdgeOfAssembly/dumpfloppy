/**
 * @file sk_heroquest_exe_file.h
 * @brief HeroQuest EXE File
 * @see https://moddingwiki.shikadi.net/wiki/HeroQuest_EXE_File
 */
#ifndef DUMPFLOPPY_FORMATS_SK_HEROQUEST_EXE_FILE_H
#define DUMPFLOPPY_FORMATS_SK_HEROQUEST_EXE_FILE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_heroquest_exe_file final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "HEROQUEST FILE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/HeroQuest_EXE_File";
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
