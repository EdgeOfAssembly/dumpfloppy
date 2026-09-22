/**
 * @file sk_selectware_archive.h
 * @brief SelectWare Archive
 * @see https://moddingwiki.shikadi.net/wiki/SelectWare_Archive
 */
#ifndef DUMPFLOPPY_FORMATS_SK_SELECTWARE_ARCHIVE_H
#define DUMPFLOPPY_FORMATS_SK_SELECTWARE_ARCHIVE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_selectware_archive final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SELECTWARE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/SelectWare_Archive";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        static constexpr uint8_t k_m0_0[]{0x53, 0x65, 0x6C, 0x65, 0x63, 0x74, 0x57, 0x61, 0x72, 0x65, 0x20, 0x54, 0x65, 0x63, 0x68, 0x6E, 0x6F, 0x6C, 0x6F, 0x67, 0x69, 0x65, 0x73, 0x20, 0x64, 0x65, 0x6D, 0x6F, 0x20, 0x66, 0x69, 0x6C, 0x65};
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
