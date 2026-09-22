/**
 * @file sk_pic.h
 * @brief PIC Format
 * @see https://moddingwiki.shikadi.net/wiki/PIC_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_PIC_H
#define DUMPFLOPPY_FORMATS_SK_PIC_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_pic final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "PIC";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/PIC_Format";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        static constexpr uint8_t k_m0_0[]{0x50, 0x49, 0x43, 0x00};
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
