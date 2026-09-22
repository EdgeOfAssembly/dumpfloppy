/**
 * @file sk_raw_adlib.h
 * @brief RAW Format (Adlib)
 * @see https://moddingwiki.shikadi.net/wiki/RAW_Format_(Adlib)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_RAW_ADLIB_H
#define DUMPFLOPPY_FORMATS_SK_RAW_ADLIB_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_raw_adlib final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "RAW ADLIB";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/RAW_Format_(Adlib)";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        static constexpr uint8_t k_m0_0[]{0x52, 0x41, 0x57, 0x41, 0x44, 0x41, 0x54, 0x41};
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
