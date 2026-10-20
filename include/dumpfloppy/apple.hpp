/**
 * @file apple.hpp
 * @brief Apple DOS 3.3 (VTOC T17/S0) and ProDOS (volume dir block 2) parsers.
 *
 * 140K DOS 3.3 and 140K ProDOS share the 143360-byte size; VTOC vs the
 * ProDOS $F volume header distinguishes them. @ref analyse stores the
 * result in @c analysis::apple.
 */
#ifndef DUMPFLOPPY_APPLE_HPP
#define DUMPFLOPPY_APPLE_HPP

#include "dumpfloppy/apple_view.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace dumpfloppy
{

/** @brief DOS 3.3 sector size. */
inline constexpr uint32_t k_apple_dos_sector = 256u;

/** @brief ProDOS block size. */
inline constexpr uint32_t k_apple_prodos_block = 512u;

/** @brief Standard 35-track 16-sector DOS 3.3 image. */
inline constexpr std::size_t k_apple_dos33_140k = 143360u;

/**
 * @brief Parse DOS 3.3 or ProDOS from a raw volume (or 2IMG payload).
 *
 * @param[in] data Sector/block image, not the 2IMG prefix.
 */
[[nodiscard]] apple_disk parse_apple(std::span<const uint8_t> data);

/** @brief Sector-order hint for @ref parse_apple_ordered. */
enum class apple_order : uint8_t
{
    unknown = 0, /**< Try ProDOS order, then DOS order (raw .dsk). */
    prodos = 1,  /**< ProDOS block order (.po, 2IMG format 1). */
    dos = 2      /**< DOS 3.3 sector order (.do, 2IMG format 0). */
};

/**
 * @brief Like @ref parse_apple, but also finds ProDOS in a DOS-order image.
 *
 * DOS 3.3 is tried first. ProDOS is then tried in @p order first; with
 * @c unknown both orders are tried. A remapped volume sets
 * @c apple_disk::dos_order and stores the ProDOS-order copy.
 *
 * @param[in] data  Sector/block image, not the 2IMG prefix.
 * @param[in] order Sector-order hint from the container or extension.
 */
[[nodiscard]] apple_disk parse_apple_ordered(std::span<const uint8_t> data,
                                             apple_order order);

/**
 * @brief Rearrange a 16-sector DOS-order 5.25 image into ProDOS block order.
 *
 * Each track's 16×256-byte DOS sectors become ProDOS pairs (block 0 is
 * DOS S0+S2). Used after WOZ 6-and-2 decode, which stores address-field
 * T/S as DOS order.
 *
 * @param[in] dos_order Image whose size is a multiple of 16×256.
 *
 * @return ProDOS-order copy, or empty when the size is not a 16-sector track map.
 */
[[nodiscard]] std::vector<uint8_t> apple_dos_order_to_prodos(
    std::span<const uint8_t> dos_order);

/**
 * @brief Host extract name (spaces stripped; `/` flattened).
 *
 * @param[in] file Directory slot.
 */
[[nodiscard]] std::string apple_host_filename(const apple_file& file);

/**
 * @brief Read file payload from @a disk.volume.
 *
 * DOS 3.3 follows the T/S list. ProDOS reads seedling, sapling, and tree
 * files (sparse pointers read as zeros) and either fork of an extended file;
 * the result is exactly EOF bytes. Directories return empty.
 *
 * @param[in] disk Parsed volume.
 * @param[in] file Directory slot.
 */
[[nodiscard]] std::vector<uint8_t> read_apple_file(const apple_disk& disk,
                                                   const apple_file& file);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_APPLE_HPP */
