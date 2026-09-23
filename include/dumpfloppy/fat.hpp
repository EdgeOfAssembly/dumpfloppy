/**
 * @file fat.hpp
 * @brief FAT table walk, copy compare, free/bad/orphan accounting.
 */
#ifndef DUMPFLOPPY_FAT_HPP
#define DUMPFLOPPY_FAT_HPP

#include "dumpfloppy/fat_view.hpp"
#include "dumpfloppy/types.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace dumpfloppy
{

/**
 * @brief Read one FAT entry (FAT12 or FAT16).
 *
 * @retval true  @p out filled.
 * @retval false Truncated table or unsupported kind.
 */
[[nodiscard]] bool fat_get(std::span<const uint8_t> fat, fat_kind kind,
                           uint32_t cluster, uint16_t& out);

/**
 * @brief Write one FAT entry (FAT12 or FAT16).
 */
[[nodiscard]] bool fat_set(std::span<uint8_t> fat, fat_kind kind, uint32_t cluster,
                           uint16_t value);

/**
 * @brief Walk a cluster chain from @p start until EOC, bad, or cycle.
 *
 * @param[in]  fat         One FAT copy.
 * @param[in]  kind        FAT12 or FAT16.
 * @param[in]  start       First cluster (0 means empty file).
 * @param[in]  max_cluster Inclusive last valid data cluster.
 * @param[out] notes       Cycle / bad / truncated remarks.
 */
[[nodiscard]] std::vector<uint16_t>
walk_chain(std::span<const uint8_t> fat, fat_kind kind, uint16_t start,
           uint32_t max_cluster, std::string& notes);

/**
 * @brief Account free/bad/allocated and compare FAT copies.
 */
[[nodiscard]] fat_summary summarise_fat(std::span<const uint8_t> image,
                                        const bpb_info& bpb, fat_kind kind);

/**
 * @brief Byte offset of cluster @p cluster (2-based) in the image.
 *
 * @return @c SIZE_MAX (`std::size_t(-1)`) if the cluster is out of range.
 *         Callers must treat @c off >= image size as invalid.
 */
[[nodiscard]] std::size_t cluster_offset(const bpb_info& bpb, uint32_t cluster);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_FAT_HPP */
