/**
 * @file format_container.cpp
 * @brief Disk-image container catalog (HxC, 86F, D64, G64, IMA, …).
 */
#include "dumpfloppy/format_registry.hpp"
#include "dumpfloppy/formats/box86f.h"
#include "dumpfloppy/formats/generated_container.h"
#include "dumpfloppy/formats/hxc_mfm.h"

namespace dumpfloppy
{
namespace
{

const formats::hxc_mfm k_hxc_mfm{};
const formats::box86f k_box86f{};

} /* namespace */

const std::vector<const file_format*>& container_formats()
{
    static const std::vector<const file_format*> k_all = []()
    {
        std::vector<const file_format*> out;
        out.push_back(&k_hxc_mfm);
        out.push_back(&k_box86f);
        append_generated_container(out);
        return out;
    }();
    return k_all;
}

} /* namespace dumpfloppy */
