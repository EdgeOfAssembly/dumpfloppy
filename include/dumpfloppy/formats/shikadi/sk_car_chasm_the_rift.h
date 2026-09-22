/**
 * @file sk_car_chasm_the_rift.h
 * @brief CAR Format (Chasm: The Rift)
 * @see https://moddingwiki.shikadi.net/wiki/CAR_Format_(Chasm:_The_Rift)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CAR_CHASM_THE_RIFT_H
#define DUMPFLOPPY_FORMATS_SK_CAR_CHASM_THE_RIFT_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_car_chasm_the_rift final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CAR CHASM";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/CAR_Format_(Chasm:_The_Rift)";
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
