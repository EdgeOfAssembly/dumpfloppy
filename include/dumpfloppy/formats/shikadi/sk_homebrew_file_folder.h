/**
 * @file sk_homebrew_file_folder.h
 * @brief HomeBrew File Folder Format
 * @see https://moddingwiki.shikadi.net/wiki/HomeBrew_File_Folder_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_HOMEBREW_FILE_FOLDER_H
#define DUMPFLOPPY_FORMATS_SK_HOMEBREW_FILE_FOLDER_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_homebrew_file_folder final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "HOMEBREW";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/HomeBrew_File_Folder_Format";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        static constexpr uint8_t k_m0_0[]{0x48, 0x6F, 0x6D, 0x65, 0x42, 0x72, 0x65, 0x77, 0x20, 0x46, 0x69, 0x6C, 0x65, 0x20, 0x46, 0x6F, 0x6C, 0x64, 0x65, 0x72};
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
