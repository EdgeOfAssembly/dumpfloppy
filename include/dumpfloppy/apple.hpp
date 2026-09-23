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

/**
 * @brief Host extract name (spaces stripped; `/` flattened).
 *
 * @param[in] file Directory slot.
 */
[[nodiscard]] std::string apple_host_filename(const apple_file& file);

/**
 * @brief Read file payload from @a disk.volume.
 *
 * DOS 3.3 follows the T/S list. ProDOS reads seedling and sapling files;
 * tree files return empty.
 *
 * @param[in] disk Parsed volume.
 * @param[in] file Directory slot.
 */
[[nodiscard]] std::vector<uint8_t> read_apple_file(const apple_disk& disk,
                                                   const apple_file& file);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_APPLE_HPP */
