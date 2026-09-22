/**
 * @file sk_nomad_3d_model.h
 * @brief Nomad 3D Model Format
 * @see https://moddingwiki.shikadi.net/wiki/Nomad_3D_Model_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_NOMAD_3D_MODEL_H
#define DUMPFLOPPY_FORMATS_SK_NOMAD_3D_MODEL_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_nomad_3d_model final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "NOMAD 3D MODEL";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Nomad_3D_Model_Format";
    }

    [[nodiscard]] format_kind kind() const override
    {
        return format_kind::file;
    }

    [[nodiscard]] bool detect(std::span<const uint8_t> data) const override
    {
        (void)data;
        return false;
    }
};

} /* namespace formats */
} /* namespace dumpfloppy */

#endif
