/**
 * @file sk_lbm.h
 * @brief LBM Format
 * @see https://moddingwiki.shikadi.net/wiki/LBM_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_LBM_H
#define DUMPFLOPPY_FORMATS_SK_LBM_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_lbm final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "LBM";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/LBM_Format";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        static constexpr uint8_t k_m0_0[]{0x46, 0x4F, 0x52, 0x4D};
        static constexpr uint8_t k_m0_1[]{0x49, 0x4C, 0x42, 0x4D};
        static constexpr uint8_t k_m1_0[]{0x46, 0x4F, 0x52, 0x4D};
        static constexpr uint8_t k_m1_1[]{0x50, 0x42, 0x4D, 0x20};
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
        return (match(data, 0u, k_m0_0, sizeof(k_m0_0)) && match(data, 8u, k_m0_1, sizeof(k_m0_1)))
            || (match(data, 0u, k_m1_0, sizeof(k_m1_0)) && match(data, 8u, k_m1_1, sizeof(k_m1_1)));
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
