/**
 * @file sk_viacom_new_media_graphics_file.h
 * @brief Viacom New Media Graphics File Format
 * @see https://moddingwiki.shikadi.net/wiki/Viacom_New_Media_Graphics_File_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_VIACOM_NEW_MEDIA_GRAPHICS_FILE_H
#define DUMPFLOPPY_FORMATS_SK_VIACOM_NEW_MEDIA_GRAPHICS_FILE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_viacom_new_media_graphics_file final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "VNM GRAPHIC";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Viacom_New_Media_Graphics_File_Format";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        static constexpr uint8_t k_m0_0[]{0x56, 0x4E, 0x4D, 0x1A};
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
