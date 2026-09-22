/**
 * @file bpb.hpp
 * @brief BIOS Parameter Block and DOS 4+ extended BPB (serial / label).
 */
#ifndef DUMPFLOPPY_BPB_HPP
#define DUMPFLOPPY_BPB_HPP

#include "dumpfloppy/types.hpp"

#include <span>
#include <string>
#include <vector>

namespace dumpfloppy
{

/**
 * @brief Parse the 512-byte boot sector prefix as a FAT BPB.
 *
 * @param[in] boot First sector (may be shorter than 512 on a truncated image).
 */
[[nodiscard]] bpb_info parse_bpb(std::span<const uint8_t> boot);

/**
 * @brief Parse the extended BPB (volume serial, label, FS type).
 *
 * @param[in] boot Boot sector bytes.
 * @param[in] bpb  Already-parsed classic BPB (OEM / jump used as context).
 *
 * DOS 3.3 volumes (Elvira, many 720K games) have no 0x28/0x29 signature;
 * boot code occupies 0x24 onward and must not be read as a serial.
 */
[[nodiscard]] ebpb_info parse_ebpb(std::span<const uint8_t> boot, const bpb_info& bpb);

/**
 * @brief Microsoft FAT type from count of data clusters.
 *
 * @param[in] bpb Parsed BPB with @a total_sectors filled.
 */
[[nodiscard]] fat_kind fat_kind_from_bpb(const bpb_info& bpb);

/**
 * @brief Root-directory size in sectors (FAT12/16).
 */
[[nodiscard]] uint32_t root_dir_sectors(const bpb_info& bpb);

/**
 * @brief First data sector (cluster 2) relative to the volume start.
 */
[[nodiscard]] uint32_t first_data_sector(const bpb_info& bpb);

/**
 * @brief Count of data clusters (not including 0/1).
 */
[[nodiscard]] uint32_t data_cluster_count(const bpb_info& bpb);

/**
 * @brief Human description of the 3-byte jump at offset 0.
 */
[[nodiscard]] std::string describe_jump(const uint8_t jump[3]);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_BPB_HPP */
