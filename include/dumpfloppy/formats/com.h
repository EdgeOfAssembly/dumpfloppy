/**
 * @file com.h
 * @brief MS-DOS .COM executable (no on-disk magic; loaded at CS:0100h).
 * @see http://fileformats.archiveteam.org/wiki/COM
 */
#ifndef DUMPFLOPPY_FORMATS_COM_H
#define DUMPFLOPPY_FORMATS_COM_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

/**
 * @brief DOS COM. Type comes from the `.COM` extension; magic does not apply.
 */
class com final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "COM";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/COM";
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

    [[nodiscard]] bool match_name(std::string_view name) const override
    {
        return name_has_extension(name, "COM");
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_FORMATS_COM_H */
