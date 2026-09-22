/**
 * @file at_dmf.h
 * @brief Microsoft Distribution Media Format (1680K floppy).
 * @see http://fileformats.archiveteam.org/wiki/DMF_(Distribution_Media_Format)
 */
#ifndef DUMPFLOPPY_FORMATS_AT_DMF_H
#define DUMPFLOPPY_FORMATS_AT_DMF_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_dmf final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "PC DMF";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/DMF_(Distribution_Media_Format)";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
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
