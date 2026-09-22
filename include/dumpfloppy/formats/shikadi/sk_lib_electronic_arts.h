/**
 * @file sk_lib_electronic_arts.h
 * @brief LIB Format (Electronic Arts)
 * @see https://moddingwiki.shikadi.net/wiki/LIB_Format_(Electronic_Arts)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_LIB_ELECTRONIC_ARTS_H
#define DUMPFLOPPY_FORMATS_SK_LIB_ELECTRONIC_ARTS_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_lib_electronic_arts final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "LIB EA";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/LIB_Format_(Electronic_Arts)";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        static constexpr uint8_t k_m0_0[]{0x45, 0x41, 0x4C, 0x49, 0x42};
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
        return match(data, 0u, k_m0_0, sizeof(k_m0_0));
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
