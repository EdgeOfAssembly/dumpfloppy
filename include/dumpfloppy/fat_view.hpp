/**
 * @file fat_view.hpp
 * @brief FAT summary type for @ref analysis (no table-walk API).
 */
#ifndef DUMPFLOPPY_FAT_VIEW_HPP
#define DUMPFLOPPY_FAT_VIEW_HPP

#include "dumpfloppy/types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace dumpfloppy
{

/** @brief Summary of both FAT copies and cluster use. */
struct fat_summary
{
    fat_kind kind = fat_kind::unknown;
    uint32_t fat_bytes = 0;
    uint32_t cluster_count = 0; /**< Data clusters (2 .. 2+n-1). */
    uint32_t max_cluster = 0;   /**< Inclusive last valid cluster index. */
    uint32_t free_clusters = 0;
    uint32_t bad_clusters = 0;
    uint32_t reserved_clusters = 0;
    uint32_t eof_markers = 0;
    uint32_t allocated_clusters = 0;
    bool copies_match = true;
    uint32_t copy_mismatch_bytes = 0;
    uint16_t fat0_media = 0; /**< FAT[0] 12/16-bit entry. */
    uint16_t fat0_eoc = 0;   /**< FAT[1] typically EOC. */
    std::vector<uint16_t> bad_list{};
    std::string media_note{};
};

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_FAT_VIEW_HPP */
