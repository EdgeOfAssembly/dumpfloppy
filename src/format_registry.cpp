/**
 * @file format_registry.cpp
 * @brief Format table (core + generated extras).
 */
#include "dumpfloppy/format_registry.hpp"
#include "dumpfloppy/formats/com.h"
#include "dumpfloppy/formats/fat12.h"
#include "dumpfloppy/formats/pkd.h"

#include <iterator>
#include <vector>

#if defined(__has_include)
#  if __has_include("dumpfloppy/formats/generated_formats.h")
#    include "dumpfloppy/formats/generated_formats.h"
#    define DUMPFLOPPY_HAVE_GENERATED_FORMATS 1
#  endif
#endif

namespace dumpfloppy
{
namespace
{

const formats::fat12 k_fat12{};
const formats::pkd k_pkd{};
const formats::com k_com{};

} /* namespace */

std::vector<const file_format*> all_formats()
{
    std::vector<const file_format*> out;
    out.push_back(&k_fat12);
    out.push_back(&k_pkd);
    out.push_back(&k_com);
#ifdef DUMPFLOPPY_HAVE_GENERATED_FORMATS
    append_generated_formats(out);
#endif
    return out;
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
