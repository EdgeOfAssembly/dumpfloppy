/**
 * @file format_payload.cpp
 * @brief In-volume file format catalog (PKD, COM, ARC, Shikadi payloads).
 */
#include "dumpfloppy/format_registry.hpp"
#include "dumpfloppy/formats/arc.h"
#include "dumpfloppy/formats/com.h"
#include "dumpfloppy/formats/generated_payload.h"
#include "dumpfloppy/formats/pkd.h"
#include "dumpfloppy/formats/pop_arc.h"

namespace dumpfloppy
{
namespace
{

const formats::pkd k_pkd{};
const formats::com k_com{};
const formats::sea_arc k_sea_arc{};
const formats::pop_arc k_pop_arc{};

} /* namespace */

const std::vector<const file_format*>& payload_formats()
{
    static const std::vector<const file_format*> k_all = []()
    {
        std::vector<const file_format*> out;
        out.push_back(&k_pkd);
        out.push_back(&k_com);
        out.push_back(&k_sea_arc);
        out.push_back(&k_pop_arc);
        append_generated_payload(out);
        return out;
    }();
    return k_all;
}

} /* namespace dumpfloppy */
