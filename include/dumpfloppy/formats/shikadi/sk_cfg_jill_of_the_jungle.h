/**
 * @file sk_cfg_jill_of_the_jungle.h
 * @brief CFG Format (Jill of the Jungle)
 * @see https://moddingwiki.shikadi.net/wiki/CFG_Format_(Jill_of_the_Jungle)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CFG_JILL_OF_THE_JUNGLE_H
#define DUMPFLOPPY_FORMATS_SK_CFG_JILL_OF_THE_JUNGLE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_cfg_jill_of_the_jungle final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CFG JILL";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/CFG_Format_(Jill_of_the_Jungle)";
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
