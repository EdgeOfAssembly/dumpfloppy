/**
 * @file unused.hpp
 * @brief Scan FAT-free and FAT-bad clusters for leftover payloads.
 */
#ifndef DUMPFLOPPY_UNUSED_HPP
#define DUMPFLOPPY_UNUSED_HPP

#include "dumpfloppy/bpb.hpp"
#include "dumpfloppy/types.hpp"
#include "dumpfloppy/unused_view.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace dumpfloppy
{

/**
 * @brief Walk data clusters and collect leftover runs.
 *
 * Consecutive interesting free clusters become one run; bad clusters are
 * grouped separately from free. Empty (all-zero) and DOS format-fill
 * (`0xF6`) clusters are skipped.
 *
 * @param[in] volume Logical FAT volume (assembled CHS or raw img/ima).
 * @param[in] bpb    Parsed BPB (cluster size and first data LBA).
 * @param[in] kind   FAT12 or FAT16.
 *
 * @return Runs in cluster order. Empty when the volume is not FAT.
 */
[[nodiscard]] std::vector<unused_run>
scan_unused_clusters(std::span<const uint8_t> volume, const bpb_info& bpb,
                     fat_kind kind);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_UNUSED_HPP */
