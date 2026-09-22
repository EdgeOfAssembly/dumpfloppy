/**
 * @file sk_halloween_harry_dialogue.h
 * @brief Halloween Harry Dialogue Format
 * @see https://moddingwiki.shikadi.net/wiki/Halloween_Harry_Dialogue_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_HALLOWEEN_HARRY_DIALOGUE_H
#define DUMPFLOPPY_FORMATS_SK_HALLOWEEN_HARRY_DIALOGUE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_halloween_harry_dialogue final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "HALLOWEEN DIALOG";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Halloween_Harry_Dialogue_Format";
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
