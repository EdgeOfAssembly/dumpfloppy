/**
 * @file sk_legend_of_seven_paladins_3d_archive.h
 * @brief Legend of Seven Paladins 3D Archive Format
 * @see https://moddingwiki.shikadi.net/wiki/Legend_of_Seven_Paladins_3D_Archive_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_LEGEND_OF_SEVEN_PALADINS_3D_ARCHIVE_H
#define DUMPFLOPPY_FORMATS_SK_LEGEND_OF_SEVEN_PALADINS_3D_ARCHIVE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_legend_of_seven_paladins_3d_archive final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "LOSP ARCHIVE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Legend_of_Seven_Paladins_3D_Archive_Format";
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
