/**
 * @file format_filesystem.cpp
 * @brief On-disk filesystem catalog (FAT12, CBMFS, OFS, FFS, …).
 */
#include "dumpfloppy/format_registry.hpp"
#include "dumpfloppy/formats/fat12.h"
#include "dumpfloppy/formats/generated_filesystem.h"

namespace dumpfloppy
{
namespace
{

const formats::fat12 k_fat12{};

} /* namespace */

const std::vector<const file_format*>& filesystem_formats()
{
    static const std::vector<const file_format*> k_all = []()
    {
        std::vector<const file_format*> out;
        out.push_back(&k_fat12);
        append_generated_filesystem(out);
        return out;
    }();
    return k_all;
}

} /* namespace dumpfloppy */
