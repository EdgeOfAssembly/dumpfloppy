/**
 * @file at_ipf.h
 * @brief Software Preservation Society IPF flux image.
 * @see http://fileformats.archiveteam.org/wiki/IPF
 */
#ifndef DUMPFLOPPY_FORMATS_AT_IPF_H
#define DUMPFLOPPY_FORMATS_AT_IPF_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class at_ipf final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SPS IPF";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/IPF";
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
        static constexpr uint8_t k_magic[] = {0x43u, 0x41u, 0x50u, 0x53u, 0x00u, 0x00u, 0x00u, 0x0cu};
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
