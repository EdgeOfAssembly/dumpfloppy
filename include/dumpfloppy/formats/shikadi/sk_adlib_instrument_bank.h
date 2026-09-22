/**
 * @file sk_adlib_instrument_bank.h
 * @brief AdLib Instrument Bank Format
 * @see https://moddingwiki.shikadi.net/wiki/AdLib_Instrument_Bank_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_ADLIB_INSTRUMENT_BANK_H
#define DUMPFLOPPY_FORMATS_SK_ADLIB_INSTRUMENT_BANK_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_adlib_instrument_bank final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "ADLIB BANK";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/AdLib_Instrument_Bank_Format";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        static constexpr uint8_t k_m0_0[]{0x41, 0x44, 0x4C, 0x49, 0x42, 0x2D};
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
        return match(data, 2u, k_m0_0, sizeof(k_m0_0));
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
