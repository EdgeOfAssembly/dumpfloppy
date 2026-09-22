/**
 * @file pop_arc.h
 * @brief Bullfrog/EA Populous installer archive (`.ARC` on the 360K DOS disk).
 *
 * Not SEA ARC (`0x1A`). Layout:
 *   uint16  kind (1 = POPULOUS.ARC, 2 = CGA/EGA/VGA.ARC)
 *   uint32  unknown
 *   repeating: NUL-terminated DOS path, uint16 flags, uint32 size
 *   then packed payloads
 *
 * @see http://fileformats.archiveteam.org/wiki/ARC_(compression_format)
 */
#ifndef DUMPFLOPPY_FORMATS_POP_ARC_H
#define DUMPFLOPPY_FORMATS_POP_ARC_H

#include "dumpfloppy/format.h"

#include <cstddef>
#include <cstdint>

namespace dumpfloppy
{
namespace formats
{

class pop_arc final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "POP ARC";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "http://fileformats.archiveteam.org/wiki/ARC_(compression_format)";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        if (data.size() < 16u)
        {
            return false;
        }
        const uint16_t kind =
            static_cast<uint16_t>(data[0] | (static_cast<uint16_t>(data[1]) << 8));
        if (kind != 1u && kind != 2u)
        {
            return false;
        }
        size_t p = 6;
        unsigned files = 0;
        while (p + 8u < data.size() && files < 64u)
        {
            const uint8_t c = data[p];
            if (c < 0x20u || c > 0x7Eu)
            {
                break;
            }
            size_t nend = p;
            while (nend < data.size() && data[nend] != 0u)
            {
                const uint8_t ch = data[nend];
                if (ch < 0x20u || ch > 0x7Eu)
                {
                    return false;
                }
                ++nend;
                if (nend - p > 64u)
                {
                    return false;
                }
            }
            if (nend >= data.size() || nend == p)
            {
                return false;
            }
            ++files;
            p = nend + 1u + 6u; /* flags uint16 + size uint32 */
        }
        return files >= 1u;
    }

    [[nodiscard]] bool match_name(std::string_view name) const override
    {
        return name_has_extension(name, "ARC");
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_FORMATS_POP_ARC_H */
