/**
 * @file apple_view.hpp
 * @brief Apple DOS 3.3 / ProDOS result types for @ref analysis (no parser).
 */
#ifndef DUMPFLOPPY_APPLE_VIEW_HPP
#define DUMPFLOPPY_APPLE_VIEW_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace dumpfloppy
{

/** @brief Detected Apple filesystem. */
enum class apple_fs : uint8_t
{
    none = 0,
    dos33 = 1,  /**< DOS 3.3, 256-byte sectors. */
    prodos = 2  /**< ProDOS, 512-byte blocks. */
};

/**
 * @brief One DOS 3.3 or ProDOS directory slot.
 */
struct apple_file
{
    std::string name{};
    std::string type_name{}; /**< TXT / BIN / BAS / DIR / … */
    uint8_t type_byte = 0;
    bool deleted = false;
    bool locked = false;
    uint32_t byte_size = 0;  /**< DOS sector-count×256 or ProDOS EOF. */
    uint16_t key = 0;        /**< DOS T/S list packed, or ProDOS key block. */
    uint8_t storage = 0;     /**< ProDOS storage type 1–3 / 0xD; 0 on DOS 3.3. */
    uint8_t ts_track = 0;    /**< DOS 3.3 track/sector list. */
    uint8_t ts_sector = 0;
};

/**
 * @brief Parsed Apple volume; @a present is false when VTOC/header is unusable.
 *
 * @a volume holds the 256-byte (DOS) or 512-byte (ProDOS) image used by
 * @ref read_apple_file. For 2IMG this is the payload after the prefix.
 */
struct apple_disk
{
    bool present = false;
    apple_fs fs = apple_fs::none;
    std::string fs_name{};     /**< `DOS 3.3` or `ProDOS`. */
    std::string volume_name{};
    uint8_t volume_number = 0; /**< DOS 3.3 VTOC volume (1–254). */
    uint16_t total_blocks = 0; /**< ProDOS total_blocks. */
    uint8_t tracks = 0;
    uint8_t sectors_per_track = 0;
    std::vector<apple_file> entries{};
    std::vector<uint8_t> volume{};
};

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_APPLE_VIEW_HPP */
