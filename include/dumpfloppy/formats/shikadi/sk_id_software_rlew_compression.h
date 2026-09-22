/**
 * @file sk_id_software_rlew_compression.h
 * @brief Id Software RLEW compression
 * @see https://moddingwiki.shikadi.net/wiki/Id_Software_RLEW_compression
 */
#ifndef DUMPFLOPPY_FORMATS_SK_ID_SOFTWARE_RLEW_COMPRESSION_H
#define DUMPFLOPPY_FORMATS_SK_ID_SOFTWARE_RLEW_COMPRESSION_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_id_software_rlew_compression final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "RLEW";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Id_Software_RLEW_compression";
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
