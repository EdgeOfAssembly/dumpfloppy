/**
 * @file amiga_view.hpp
 * @brief OFS/FFS result types for @ref analysis (no parser).
 */
#ifndef DUMPFLOPPY_AMIGA_VIEW_HPP
#define DUMPFLOPPY_AMIGA_VIEW_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace dumpfloppy
{

/**
 * @brief One OFS/FFS directory entry (file or subdirectory).
 *
 * @a path is Amiga-style `Dir/Name` (no volume prefix). Directories have
 * @a is_dir set; @ref read_amiga_file returns empty for them.
 */
struct amiga_file
{
    std::string path{};        /**< Relative path (`README` or `Sub/Inner`). */
    std::string name{};        /**< BCPL component name. */
    uint32_t header_block = 0; /**< File/dir header sector. */
    uint32_t parent_block = 0; /**< Parent directory sector. */
    uint32_t byte_size = 0;    /**< File size at header offset 0x144. */
    bool is_dir = false;       /**< ST_USERDIR. */
};

/**
 * @brief Parsed ADF; @a present is false when size, boot, or root is unusable.
 */
struct amiga_disk
{
    bool present = false;
    bool ffs = false; /**< Odd DOS type (DOS\\1 / DOS\\3 / DOS\\5). */
    uint8_t dos_type = 0; /**< Bootblock byte 3 (0–5). */
    std::string volume_name{};
    uint32_t root_block = 0;
    uint32_t sector_count = 0;
    std::vector<amiga_file> entries{};
};

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_AMIGA_VIEW_HPP */
