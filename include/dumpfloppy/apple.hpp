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

/** @brief Damage found while reading one file (see @ref read_apple_file). */
struct apple_read_issues
{
    std::vector<std::string> errors{}; /**< Fabricated or capped bytes: exit nonzero. */
    std::vector<std::string> notes{};  /**< Benign adjustments (trailing sparse trim). */
};

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
 * @brief Rearrange a 16-sector DOS-order (.do/.dsk) image into ProDOS block order.
 *
 * ProDOS-order sector s of each track is DOS-order sector
 * {0,14,13,12,11,10,9,8,7,6,5,4,3,2,1,15}[s], so block 0 is DOS S0+S14.
 *
 * @param[in] dos_order Image whose size is a multiple of 16×256.
 *
 * @return ProDOS-order copy, or empty when the size is not a 16-sector track map.
 */
[[nodiscard]] std::vector<uint8_t> apple_dos_order_to_prodos(
    std::span<const uint8_t> dos_order);

/**
 * @brief Rearrange a physical-sector-order image (WOZ address-field
 *        sector numbers) into ProDOS block order: ProDOS sector s is
 *        physical sector {0,2,4,…,14,1,3,…,15}[s].
 *
 * @param[in] physical Image whose size is a multiple of 16×256.
 */
[[nodiscard]] std::vector<uint8_t> apple_physical_order_to_prodos(
    std::span<const uint8_t> physical);

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
 * output is at most the volume size and stops at the last allocated
 * block. Out-of-range pointers read as zeros and are reported in
 * @p issues (errors); trimming is reported as a note. Directories return empty.
 *
 * @param[in] disk Parsed volume.
 * @param[in] file Directory slot.
 */
[[nodiscard]] std::vector<uint8_t> read_apple_file(const apple_disk& disk,
                                                   const apple_file& file,
                                                   apple_read_issues* issues = nullptr);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_APPLE_HPP */
