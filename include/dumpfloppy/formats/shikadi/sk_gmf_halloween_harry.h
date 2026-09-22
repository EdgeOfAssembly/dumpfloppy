/**
 * @file sk_gmf_halloween_harry.h
 * @brief GMF Format (Halloween Harry)
 * @see https://moddingwiki.shikadi.net/wiki/GMF_Format_(Halloween_Harry)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_GMF_HALLOWEEN_HARRY_H
#define DUMPFLOPPY_FORMATS_SK_GMF_HALLOWEEN_HARRY_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_gmf_halloween_harry final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "GMF HARRY";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/GMF_Format_(Halloween_Harry)";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        static constexpr uint8_t k_m0_0[]{0x11, 0x53, 0x75, 0x62, 0x5A, 0x65, 0x72, 0x6F, 0x20, 0x47, 0x61, 0x6D, 0x65, 0x20, 0x46, 0x69, 0x6C, 0x65};
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
