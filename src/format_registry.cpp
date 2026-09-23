/**
 * @file format_registry.cpp
 * @brief Sniff helpers over the payload / container / filesystem catalogs.
 */
#include "dumpfloppy/format_registry.hpp"

#include <vector>

namespace dumpfloppy
{

std::vector<const file_format*> all_formats()
{
    static const std::vector<const file_format*> k_all = []()
    {
        /* Core detectors first (FAT12, HxC, 86F, PKD, COM, ARC), then
         * generated catalogs. identify_format still filters by kind(). */
        std::vector<const file_format*> out;
        const auto& fs = filesystem_formats();
        const auto& cont = container_formats();
        const auto& pay = payload_formats();
        out.reserve(fs.size() + cont.size() + pay.size());
        if (!fs.empty())
        {
            out.push_back(fs.front()); /* fat12 */
        }
        if (cont.size() >= 2u)
        {
            out.push_back(cont[0]); /* hxc_mfm */
            out.push_back(cont[1]); /* box86f */
        }
        if (pay.size() >= 4u)
        {
            out.push_back(pay[0]); /* pkd */
            out.push_back(pay[1]); /* com */
            out.push_back(pay[2]); /* sea_arc */
            out.push_back(pay[3]); /* pop_arc */
        }
        if (cont.size() > 2u)
        {
            out.insert(out.end(), cont.begin() + 2, cont.end());
        }
        if (fs.size() > 1u)
        {
            out.insert(out.end(), fs.begin() + 1, fs.end());
        }
        if (pay.size() > 4u)
        {
            out.insert(out.end(), pay.begin() + 4, pay.end());
        }
        return out;
    }();
    return k_all;
}

const file_format* identify_format(std::span<const uint8_t> data, format_kind kind)
{
    for (const file_format* f : all_formats())
    {
        if (f == nullptr)
        {
            continue;
        }
        if (f->kind() != kind)
        {
            continue;
        }
        if (f->detect(data))
        {
            return f;
        }
    }
    return nullptr;
}

std::string identify_type(std::span<const uint8_t> data, format_kind kind,
                          std::string_view name)
{
    std::string label = "DATA";
    if (!name.empty())
    {
        for (const file_format* named : all_formats())
        {
            if (named == nullptr || named->kind() != kind)
            {
                continue;
            }
            if (named->match_name(name))
            {
                label = named->type();
                break;
            }
        }
    }
    const file_format* mag = identify_format(data, kind);
    if (mag != nullptr)
    {
        label = mag->type();
    }
    return clip_type_label(label);
}

} /* namespace dumpfloppy */
