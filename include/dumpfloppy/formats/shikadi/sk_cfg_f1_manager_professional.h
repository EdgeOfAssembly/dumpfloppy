/**
 * @file sk_cfg_f1_manager_professional.h
 * @brief CFG Format (F1 Manager Professional)
 * @see https://moddingwiki.shikadi.net/wiki/CFG_Format_(F1_Manager_Professional)
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CFG_F1_MANAGER_PROFESSIONAL_H
#define DUMPFLOPPY_FORMATS_SK_CFG_F1_MANAGER_PROFESSIONAL_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_cfg_f1_manager_professional final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CFG F1 MGR";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/CFG_Format_(F1_Manager_Professional)";
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
