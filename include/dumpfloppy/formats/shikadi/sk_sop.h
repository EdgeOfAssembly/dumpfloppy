/**
 * @file sk_sop.h
 * @brief SOP Format
 * @see https://moddingwiki.shikadi.net/wiki/SOP_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_SOP_H
#define DUMPFLOPPY_FORMATS_SK_SOP_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_sop final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SOP";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/SOP_Format";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        static constexpr uint8_t k_m0_0[]{0x73, 0x6F, 0x70, 0x65, 0x70, 0x6F, 0x73};
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
