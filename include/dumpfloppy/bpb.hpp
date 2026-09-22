/**
 * @file bpb.hpp
 * @brief BIOS Parameter Block and DOS 4+ extended BPB (serial / label).
 */
#ifndef DUMPFLOPPY_BPB_HPP
#define DUMPFLOPPY_BPB_HPP

#include "dumpfloppy/types.hpp"

#include <span>
#include <string>

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
 *
 * Signature 0x29 is accepted when the FS-type field looks like FATxx, or at
 * least printable ASCII; a non-printable FS type is treated as a collision
 * with boot code (same as a missing signature).
 *
 * Signature 0x28 is the serial-only EBPB (DOS 4 / Compaq) **and** the x86
 * opcode `SUB r/m8,r8`, which appears in DOS 3.x boot stubs. It is
 * @ref ebpb_info::confident only when bytes 0x27–0x2A look like a volume
 * serial (non-zero, not a typical IBM PC boot tail) **and** either:
 * - the jump/OEM look like DOS 4+ (short/near jump landing at ≥ 0x3E,
 *   printable OEM that is not a DOS 3.x id such as `MSDOS3.3` / `TAN  3.3`),
 *   or
 * - byte 0x26 is 0x28, the NT/reserved flag at 0x25 is 0, and the drive
 *   number at 0x24 is 0x00 or 0x80.
 *
 * On FAT12 (and when the FAT kind is unknown) the drive/NT prefix alone is
 * not enough: prefer no serial rather than inventing one from a 3.x stub.
 * A real DOS 4+ 0x28 floppy still matches via the jump/OEM arm.
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
