/**
 * @file sk_rsc_f1_manager_professional.h
 * @brief RSC Format (F1 Manager Professional)
 * @see https://moddingwiki.shikadi.net/wiki/RSC_Format_(F1_Manager_Professional)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_RSC_F1_MANAGER_PROFESSIONAL_H
#define DUMPFLOPPY_FORMATS_SK_RSC_F1_MANAGER_PROFESSIONAL_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_rsc_f1_manager_professional final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "RSC F1 MGR";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/RSC_Format_(F1_Manager_Professional)";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        static constexpr uint8_t k_m0_0[]{0x50, 0x52, 0x4F, 0x4C, 0x49, 0x4E, 0x45, 0x20, 0x52, 0x65, 0x73, 0x6F, 0x75, 0x72, 0x63, 0x65, 0x20, 0x46, 0x69, 0x6C, 0x65, 0x20, 0x28, 0x63, 0x29, 0x20, 0x31, 0x39, 0x39, 0x37, 0x20, 0x62, 0x79, 0x20, 0x50, 0x52, 0x4F, 0x4C, 0x49, 0x4E, 0x45, 0x20, 0x53, 0x6F, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x20, 0x47, 0x6D, 0x62, 0x48};
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
