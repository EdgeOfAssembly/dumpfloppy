/**
 * @file sk_heretic_executable.h
 * @brief Heretic Executable
 * @see https://moddingwiki.shikadi.net/wiki/Heretic_Executable
 */
#ifndef DUMPFLOPPY_FORMATS_SK_HERETIC_EXECUTABLE_H
#define DUMPFLOPPY_FORMATS_SK_HERETIC_EXECUTABLE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_heretic_executable final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "HERETIC EXECUTAB";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Heretic_Executable";
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
