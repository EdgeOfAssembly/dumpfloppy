/**
 * @file sk_titus_interactive_sqz_compression.h
 * @brief Titus Interactive SQZ Compression
 * @see https://moddingwiki.shikadi.net/wiki/Titus_Interactive_SQZ_Compression
 */
#ifndef DUMPFLOPPY_FORMATS_SK_TITUS_INTERACTIVE_SQZ_COMPRESSION_H
#define DUMPFLOPPY_FORMATS_SK_TITUS_INTERACTIVE_SQZ_COMPRESSION_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_titus_interactive_sqz_compression final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "SQZ";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Titus_Interactive_SQZ_Compression";
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
