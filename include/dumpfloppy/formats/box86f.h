/**
 * @file box86f.h
 * @brief 86Box floppy surface image (`.86f`), magic `86BF`.
 * @see https://86box.readthedocs.io/en/latest/dev/formats/86f.html
 */
#ifndef DUMPFLOPPY_FORMATS_BOX86F_H
#define DUMPFLOPPY_FORMATS_BOX86F_H

#include "dumpfloppy/format.h"

#include <cstring>

namespace dumpfloppy
{
namespace formats
{

class box86f final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "86BOX 86F";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://86box.readthedocs.io/en/latest/dev/formats/86f.html";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::disk_image;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        return data.size() >= 4u && std::memcmp(data.data(), "86BF", 4) == 0;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
