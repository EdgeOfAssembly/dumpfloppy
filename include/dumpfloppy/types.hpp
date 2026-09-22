/**
 * @file types.hpp
 * @brief Shared enums, DOS directory attributes, and parsed structures.
 */
#ifndef DUMPFLOPPY_TYPES_HPP
#define DUMPFLOPPY_TYPES_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace dumpfloppy
{

/** @brief On-disk container guessed from the file name and size. */
enum class container_kind
{
    img_raw,     /**< `.img` raw sector dump. */
    ima_winimage,/**< `.ima` WinImage raw dump (same layout as `.img`). */
    unknown_raw  /**< Explicit path with another extension. */
};

/** @brief FAT width selected by Microsoft cluster-count rules. */
enum class fat_kind
{
    unknown,
    fat12,
    fat16,
    fat32
};

/**
 * @brief How far BIOS / DOS would get if this image were booted.
 *
 * A “booter” in the DOS-game sense is @ref boot_class::custom_booter
 * (payload in the boot sector, typically no IO.SYS).
 */
enum class boot_class
{
    not_bootable,    /**< No 0x55AA and/or no jump. */
    dos_non_system,  /**< Standard DOS boot; volume lacks IO.SYS/IBMBIO. */
    dos_system,      /**< DOS boot sector and system files in root. */
    custom_booter    /**< Jump + signature, not the MS-DOS loader. */
};

/** @brief 8.3 directory attribute bits (on-disk). */
inline constexpr uint8_t k_attr_read_only = 0x01;
inline constexpr uint8_t k_attr_hidden    = 0x02;
inline constexpr uint8_t k_attr_system    = 0x04;
inline constexpr uint8_t k_attr_volume    = 0x08;
inline constexpr uint8_t k_attr_directory = 0x10;
inline constexpr uint8_t k_attr_archive   = 0x20;
inline constexpr uint8_t k_attr_lfn       = 0x0F;

/** @brief First byte of a deleted 8.3 / LFN slot. */
inline constexpr uint8_t k_dir_deleted = 0xE5;
/** @brief First byte of an unused slot; remaining slots are unused too. */
inline constexpr uint8_t k_dir_end     = 0x00;
/** @brief Pending 0xE5 (Kanji) stored as 0x05 in the first name byte. */
inline constexpr uint8_t k_dir_kanji_e5 = 0x05;

/** @brief Extended BPB boot signature with serial + label + FS type. */
inline constexpr uint8_t k_ebpb_sig_29 = 0x29;
/** @brief Extended BPB boot signature with serial only (DOS 4 / Compaq). */
inline constexpr uint8_t k_ebpb_sig_28 = 0x28;

/** @brief Geometry inferred from image length and/or BPB. */
struct geometry
{
    uint32_t cylinders = 0;
    uint32_t heads = 0;
    uint32_t sectors_per_track = 0;
    uint32_t bytes_per_sector = 512;
    uint64_t expected_bytes = 0;
    std::string media_name{};
};

/** @brief Parsed BIOS Parameter Block (FAT12/16 common prefix). */
struct bpb_info
{
    uint8_t  jump[3] = {0, 0, 0};
    std::string oem{};
    uint16_t bytes_per_sector = 0;
    uint8_t  sectors_per_cluster = 0;
    uint16_t reserved_sectors = 0;
    uint8_t  fat_count = 0;
    uint16_t root_entry_count = 0;
    uint16_t total_sectors_16 = 0;
    uint8_t  media_descriptor = 0;
    uint16_t sectors_per_fat_16 = 0;
    uint16_t sectors_per_track = 0;
    uint16_t head_count = 0;
    uint32_t hidden_sectors = 0;
    uint32_t total_sectors_32 = 0;
    uint32_t total_sectors = 0;
    bool     looks_valid = false;
    std::string invalid_reason{};
};

/** @brief DOS 4+ extended BPB (offsets 0x24–0x3D) when present. */
struct ebpb_info
{
    bool present = false;
    bool confident = false; /**< FS type looks like FATxx. */
    uint8_t drive_number = 0;
    uint8_t nt_flags = 0;
    uint8_t boot_signature = 0;
    uint32_t volume_serial = 0;
    bool has_serial = false;
    std::string volume_label{};
    bool has_label = false;
    std::string fs_type{};
};

/** @brief One 8.3 / LFN-backed directory entry (live or deleted). */
struct dir_entry
{
    std::string path{};           /**< `\\FOO\\BAR.TXT` using backslashes. */
    std::string name_83{};        /**< Display 8.3 (`FILE.TXT` / `?ILE.TXT`). */
    std::string lfn{};            /**< UTF-8 long name when present. */
    uint8_t attributes = 0;
    uint32_t size = 0;
    uint16_t first_cluster = 0;
    uint16_t write_time = 0;
    uint16_t write_date = 0;
    uint16_t create_time = 0;
    uint16_t create_date = 0;
    uint8_t  create_tenth = 0;
    uint16_t access_date = 0;
    uint8_t  nt_reserved = 0;
    bool deleted = false;
    bool is_lfn_orphan = false;   /**< LFN slots with no 8.3 tail. */
    bool after_terminator = false;/**< Non-zero slot after a 0x00 entry. */
    std::vector<uint16_t> cluster_chain{};
    std::string magic{};          /**< First-bytes identity (MZ, …). */
    std::string notes{};
};

/** @brief Volume identity gathered from EBPB and the root directory. */
struct volume_info
{
    std::optional<uint32_t> serial_ebpb{};
    std::string serial_text{};    /**< `XXXX-XXXX` or empty. */
    std::string label_ebpb{};
    std::string label_root{};
    std::string label_best{};
};

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_TYPES_HPP */
