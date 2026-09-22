/**
 * @file sk_knowledge_dynamics_lzw_compressor.h
 * @brief Knowledge Dynamics LZW COMPRESSOR
 * @see https://moddingwiki.shikadi.net/wiki/Knowledge_Dynamics_LZW_COMPRESSOR
 */
#ifndef DUMPFLOPPY_FORMATS_SK_KNOWLEDGE_DYNAMICS_LZW_COMPRESSOR_H
#define DUMPFLOPPY_FORMATS_SK_KNOWLEDGE_DYNAMICS_LZW_COMPRESSOR_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_knowledge_dynamics_lzw_compressor final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "KD LZW";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Knowledge_Dynamics_LZW_COMPRESSOR";
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
