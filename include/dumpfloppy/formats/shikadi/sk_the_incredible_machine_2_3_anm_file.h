/**
 * @file sk_the_incredible_machine_2_3_anm_file.h
 * @brief The Incredible Machine 2-3 ANM File Format
 * @see https://moddingwiki.shikadi.net/wiki/The_Incredible_Machine_2-3_ANM_File_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_THE_INCREDIBLE_MACHINE_2_3_ANM_FILE_H
#define DUMPFLOPPY_FORMATS_SK_THE_INCREDIBLE_MACHINE_2_3_ANM_FILE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_the_incredible_machine_2_3_anm_file final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "TIM ANM";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/The_Incredible_Machine_2-3_ANM_File_Format";
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
