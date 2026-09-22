/**
 * @file sk_graphics_interchange.h
 * @brief Graphics Interchange Format
 * @see https://moddingwiki.shikadi.net/wiki/Graphics_Interchange_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_GRAPHICS_INTERCHANGE_H
#define DUMPFLOPPY_FORMATS_SK_GRAPHICS_INTERCHANGE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_graphics_interchange final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "GIF";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Graphics_Interchange_Format";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        static constexpr uint8_t k_m0_0[]{0x47, 0x49, 0x46, 0x38, 0x37, 0x61};
        static constexpr uint8_t k_m1_0[]{0x47, 0x49, 0x46, 0x38, 0x39, 0x61};
        auto match = [](std::span<const uint8_t> bytes, std::size_t off,
                        const uint8_t* mag, std::size_t mag_len) -> bool
        {
            if (bytes.size() < off + mag_len)
            {
                return false;
            }
            for (std::size_t i = 0; i < mag_len; ++i)
            {
                if (bytes[off + i] != mag[i])
                {
                    return false;
                }
            }
            return true;
        };
        return match(data, 0u, k_m0_0, sizeof(k_m0_0))
            || match(data, 0u, k_m1_0, sizeof(k_m1_0));
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
