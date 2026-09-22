/**
 * @file directory.hpp
 * @brief FAT12/16 directory walk, including deleted and post-terminator slots.
 */
#ifndef DUMPFLOPPY_DIRECTORY_HPP
#define DUMPFLOPPY_DIRECTORY_HPP

#include "dumpfloppy/fat.hpp"
#include "dumpfloppy/types.hpp"

#include <span>
#include <string>
#include <vector>

namespace dumpfloppy
{

/**
 * @brief Recursively list directories.
 *
 * Deleted 8.3 entries (first byte 0xE5) are included. Slots after the first
 * 0x00 terminator are still scanned and flagged as @a after_terminator.
 *
 * Live occupancy is built once after the walk. Deleted entries never follow
 * a FAT chain owned by a live dirent; @a cluster_chain is a contiguous run
 * from @a first_cluster until a live-owned or bad cluster (empty/truncated
 * when the first cluster was reused).
 *
 * @param[in] image Whole image.
 * @param[in] bpb   Valid BPB.
 * @param[in] kind  FAT12 or FAT16.
 * @param[in] fat   FAT0 bytes.
 */
[[nodiscard]] std::vector<dir_entry>
list_directories(std::span<const uint8_t> image, const bpb_info& bpb,
                 fat_kind kind, std::span<const uint8_t> fat);

/**
 * @brief Format an 8.3 on-disk name as `NAME.EXT`, using `?` when deleted.
 */
[[nodiscard]] std::string format_name_83(const uint8_t name[11], bool deleted);

/**
 * @brief Volume label as 11-character text (no fake 8.3 dot).
 */
[[nodiscard]] std::string format_volume_label(const uint8_t name[11], bool deleted);

/**
 * @brief Compact attribute string (`RHSVDA`).
 */
[[nodiscard]] std::string format_attributes(uint8_t attr);

/**
 * @brief Identify a payload from its first bytes (MZ, PK, …).
 */
[[nodiscard]] std::string sniff_magic(std::span<const uint8_t> head);

/**
 * @brief True when @p e is a regular file (not `.` / `..` / dir / volume).
 */
[[nodiscard]] bool is_payload_file(const dir_entry& e);

/**
 * @brief Recover a file's bytes from @p e.cluster_chain.
 *
 * Stops at @p e.size or at the end of readable clusters.
 *
 * Deleted files whose @a first_cluster was reused must not follow the live
 * FAT (that would dump the new file). @ref list_directories fills an
 * occupancy-aware contiguous chain (possibly empty) and marks truncation;
 * this function does not walk the FAT and does not extend a deleted chain.
 *
 * @param[in] image Whole image.
 * @param[in] bpb   Valid BPB.
 * @param[in] e     Directory entry with @a cluster_chain filled.
 *
 * @note Call @ref list_directories first so deleted occupancy is applied.
 */
[[nodiscard]] std::vector<uint8_t>
read_file_contents(std::span<const uint8_t> image, const bpb_info& bpb,
                   const dir_entry& e);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_DIRECTORY_HPP */
