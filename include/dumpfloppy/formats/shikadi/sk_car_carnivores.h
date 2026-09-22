/**
 * @file sk_car_carnivores.h
 * @brief CAR Format (Carnivores)
 * @see https://moddingwiki.shikadi.net/wiki/CAR_Format_(Carnivores)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CAR_CARNIVORES_H
#define DUMPFLOPPY_FORMATS_SK_CAR_CARNIVORES_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_car_carnivores final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CAR CARNIVORE";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/CAR_Format_(Carnivores)";
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
