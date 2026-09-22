/**
 * @file sk_castles_ii_siege_and_conquest_cluster.h
 * @brief Castles II: Siege and Conquest Cluster Format
 * @see https://moddingwiki.shikadi.net/wiki/Castles_II:_Siege_and_Conquest_Cluster_Format
 */
#ifndef DUMPFLOPPY_FORMATS_SK_CASTLES_II_SIEGE_AND_CONQUEST_CLUSTER_H
#define DUMPFLOPPY_FORMATS_SK_CASTLES_II_SIEGE_AND_CONQUEST_CLUSTER_H

#include "dumpfloppy/format.h"

namespace dumpfloppy
{
namespace formats
{

class sk_castles_ii_siege_and_conquest_cluster final : public file_format
{
public:
    [[nodiscard]] std::string type() const override
    {
        return "CAST2 CLUSTER";
    }

    [[nodiscard]] std::string_view source_url() const override
    {
        return "https://moddingwiki.shikadi.net/wiki/Castles_II:_Siege_and_Conquest_Cluster_Format";
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
