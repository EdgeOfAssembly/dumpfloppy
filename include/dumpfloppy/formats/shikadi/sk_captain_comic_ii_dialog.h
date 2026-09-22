/**
 * @file sk_captain_comic_ii_dialog.h
 * @brief Captain Comic II Dialog Format
 * @see https://moddingwiki.shikadi.net/wiki/Captain_Comic_II_Dialog_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CAPTAIN_COMIC_II_DIALOG_H
#define DUMPFLOPPY_FORMATS_SK_CAPTAIN_COMIC_II_DIALOG_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_captain_comic_ii_dialog final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CAPTAIN DIALOG";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Captain_Comic_II_Dialog_Format";
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
