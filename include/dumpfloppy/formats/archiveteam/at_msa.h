/**
 * @file at_msa.h
 * @brief Atari ST Magic Shadow Archiver disk image.
 * @see http://fileformats.archiveteam.org/wiki/MSA_(Magic_Shadow_Archiver)
 */
#ifndef DUMPFLOPPY_FORMATS_AT_MSA_H
#define DUMPFLOPPY_FORMATS_AT_MSA_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_msa final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "ATARI MSA";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/MSA_(Magic_Shadow_Archiver)";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        if (data.empty())
        {
            return false;
        }
        static constexpr uint8_t k_magic[] = {0x0eu, 0x0fu};
        if (data.size() >= sizeof(k_magic))
        {
            bool match = true;
            for (std::size_t i = 0; i < sizeof(k_magic); ++i)
            {
                if (data[i] != k_magic[i])
                {
                    match = false;
                    break;
                }
            }
            if (match)
            {
                return true;
            }
        }
        return false;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
