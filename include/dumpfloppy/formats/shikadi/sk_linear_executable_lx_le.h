/**
 * @file sk_linear_executable_lx_le.h
 * @brief Linear Executable (LX/LE) Format
 * @see https://moddingwiki.shikadi.net/wiki/Linear_Executable_(LX/LE)_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_LINEAR_EXECUTABLE_LX_LE_H
#define DUMPFLOPPY_FORMATS_SK_LINEAR_EXECUTABLE_LX_LE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_linear_executable_lx_le final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "LX/LE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Linear_Executable_(LX/LE)_Format";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        if (data.size() < 0x40)
        {
            return false;
        }
        if (!(data[0] == static_cast<uint8_t>('M') && data[1] == static_cast<uint8_t>('Z')))
        {
            return false;
        }
        const std::uint32_t pe = static_cast<std::uint32_t>(data[0x3C])
            | (static_cast<std::uint32_t>(data[0x3D]) << 8)
            | (static_cast<std::uint32_t>(data[0x3E]) << 16)
            | (static_cast<std::uint32_t>(data[0x3F]) << 24);
        if (pe > data.size() - 2u)
        {
            return false;
        }
        const bool lx = data[pe] == static_cast<uint8_t>('L')
            && data[pe + 1] == static_cast<uint8_t>('X');
        const bool le = data[pe] == static_cast<uint8_t>('L')
            && data[pe + 1] == static_cast<uint8_t>('E');
        return lx || le;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
