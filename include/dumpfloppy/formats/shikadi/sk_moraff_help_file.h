/**
 * @file sk_moraff_help_file.h
 * @brief Moraff Help File
 * @see https://moddingwiki.shikadi.net/wiki/Moraff_Help_File
 */
#ifndef DUMPFLOPPY_FORMATS_SK_MORAFF_HELP_FILE_H
#define DUMPFLOPPY_FORMATS_SK_MORAFF_HELP_FILE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_moraff_help_file final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "MORAFF HELP FILE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Moraff_Help_File";
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
