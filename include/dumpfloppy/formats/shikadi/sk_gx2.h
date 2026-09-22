/**
 * @file sk_gx2.h
 * @brief GX2 Format
 * @see https://moddingwiki.shikadi.net/wiki/GX2_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_GX2_H
#define DUMPFLOPPY_FORMATS_SK_GX2_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_gx2 final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "GX2";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/GX2_Format";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        static constexpr uint8_t k_m0_0[]{0x47, 0x58, 0x32, 0x01};
        static constexpr uint8_t k_m0_1[]{0x53, 0x50, 0x46, 0x58};
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
        return (match(data, 0u, k_m0_0, sizeof(k_m0_0)) && match(data, 18u, k_m0_1, sizeof(k_m0_1)));
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
