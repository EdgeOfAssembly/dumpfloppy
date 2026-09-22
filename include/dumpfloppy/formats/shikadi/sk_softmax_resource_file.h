/**
 * @file sk_softmax_resource_file.h
 * @brief SoftMax Resource File Format
 * @see https://moddingwiki.shikadi.net/wiki/SoftMax_Resource_File_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_SOFTMAX_RESOURCE_FILE_H
#define DUMPFLOPPY_FORMATS_SK_SOFTMAX_RESOURCE_FILE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_softmax_resource_file final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SOFTMAX RES";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/SoftMax_Resource_File_Format";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        static constexpr uint8_t k_m0_0[]{0x28, 0x43, 0x29, 0x20, 0x53, 0x6F, 0x66, 0x74, 0x4D, 0x61, 0x78, 0x20, 0x52, 0x65, 0x73, 0x6F, 0x75, 0x72, 0x63, 0x65, 0x20, 0x46, 0x69, 0x6C, 0x65};
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
