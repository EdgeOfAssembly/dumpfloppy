/**
 * @file sk_protracker_studio_module.h
 * @brief ProTracker Studio Module
 * @see https://moddingwiki.shikadi.net/wiki/ProTracker_Studio_Module
 */
#ifndef DUMPFLOPPY_FORMATS_SK_PROTRACKER_STUDIO_MODULE_H
#define DUMPFLOPPY_FORMATS_SK_PROTRACKER_STUDIO_MODULE_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_protracker_studio_module final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "PT STUDIO";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/ProTracker_Studio_Module";
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
