/**
 * @file trd_view.hpp
 * @brief TR-DOS result types for @ref analysis (no parser).
 */
#ifndef DUMPFLOPPY_TRD_VIEW_HPP
#define DUMPFLOPPY_TRD_VIEW_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace dumpfloppy
{

/**
 * @brief One TR-DOS directory slot (live or deleted).
 *
 * Deleted entries store @c 0x01 in the first name byte; @a name keeps the
 * remaining seven characters with a leading @c '?'.
 */
struct trd_file
{
    std::string name{};      /**< 8-char name, trailing spaces stripped. */
    char type_char = 'C';    /**< Directory byte 8: B/C/D/#. */
    std::string type_name{}; /**< BASIC / CODE / DATA / PRINT. */
    bool deleted = false;    /**< First name byte was 0x01. */
    uint16_t start = 0;      /**< Bytes 9–10: CODE address or BASIC line. */
    uint16_t byte_size = 0;  /**< Bytes 11–12, little-endian. */
    uint8_t sector_count = 0;
    uint8_t start_sector = 0; /**< 0–15. */
    uint8_t start_track = 0;  /**< Logical TR-DOS track. */
};

/**
 * @brief Parsed TRD; @a present is false when size or disk-info is unusable.
 */
struct trd_disk
{
    bool present = false;
    uint8_t disk_type = 0; /**< 0x16 DS/80, 0x17 DS/40, 0x18 SS/80, 0x19 SS/40. */
    std::string disk_type_name{};
    std::string label{};
    uint8_t file_count = 0;    /**< Disk-info byte 0xE4 (live files). */
    uint8_t deleted_count = 0; /**< Disk-info byte 0xEC. */
    uint16_t free_sectors = 0;
    uint8_t first_free_sector = 0;
    uint8_t first_free_track = 0;
    uint8_t cylinders = 0;
    uint8_t sides = 0;
    std::vector<trd_file> entries{};
};

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_TRD_VIEW_HPP */
