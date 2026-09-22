/**
 * @file sk_dat_centurion_defender_of_rome.h
 * @brief DAT Format (Centurion - Defender of Rome)
 * @see https://moddingwiki.shikadi.net/wiki/DAT_Format_(Centurion_-_Defender_of_Rome)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_DAT_CENTURION_DEFENDER_OF_ROME_H
#define DUMPFLOPPY_FORMATS_SK_DAT_CENTURION_DEFENDER_OF_ROME_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_dat_centurion_defender_of_rome final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "DAT CENTURION";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/DAT_Format_(Centurion_-_Defender_of_Rome)";
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
