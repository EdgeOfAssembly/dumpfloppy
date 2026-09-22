/**
 * @file sk_eye_of_the_beholder_vcn.h
 * @brief Eye of the Beholder VCN Format
 * @see https://moddingwiki.shikadi.net/wiki/Eye_of_the_Beholder_VCN_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_EYE_OF_THE_BEHOLDER_VCN_H
#define DUMPFLOPPY_FORMATS_SK_EYE_OF_THE_BEHOLDER_VCN_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_eye_of_the_beholder_vcn final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "EOB VCN";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Eye_of_the_Beholder_VCN_Format";
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
