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
 * @brief Recover a file's bytes by walking its FAT cluster chain.
 *
 * Stops at @p e.size or at the end of readable clusters (deleted files
 * whose FAT slots were reused may be truncated).
 *
 * @param[in] image Whole image.
 * @param[in] bpb   Valid BPB.
 * @param[in] e     Directory entry with @a cluster_chain filled.
 */
[[nodiscard]] std::vector<uint8_t>
read_file_contents(std::span<const uint8_t> image, const bpb_info& bpb,
                   const dir_entry& e);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_DIRECTORY_HPP */
