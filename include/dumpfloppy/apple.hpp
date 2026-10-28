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
 * @brief Rearrange a physical-sector-order image into DOS 3.3 sector order:
 *        DOS logical sector L is physical {0,13,11,9,7,5,3,1,14,12,10,8,6,4,2,15}[L].
 *
 * @param[in] physical Image whose size is a multiple of 16×256.
 */
[[nodiscard]] std::vector<uint8_t> apple_physical_order_to_dos(
    std::span<const uint8_t> physical);

/**
 * @brief Host extract name (`/` flattened; deleted files start with `?`).
 *
 * @param[in] file Directory slot.
 */
[[nodiscard]] std::string apple_host_filename(const apple_file& file);

/**
 * @brief Read file payload from @a disk.volume.
 *
 * DOS 3.3 follows the T/S list chain (holes read as zeros) and trims I/A/B
 * files to header + length word; deleted files are read from the T/S list
 * kept in the last name byte (damage there is a note, not an error). ProDOS reads seedling, sapling, and tree
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

/** @brief Outcome of `--undelete` for one DOS 3.3 catalog entry. */
enum class dos33_recovery : uint8_t
{
    none = 0,          /**< Live entry, or not DOS 3.3: nothing to recover. */
    full = 1,          /**< Every list and data sector is still free in the VTOC map. */
    partial = 2,       /**< Recovered, but some bytes may belong to a later file. */
    unrecoverable = 3  /**< No usable T/S list or no data sector left. */
};

/** @brief Result of @ref undelete_dos33. */
struct dos33_undelete
{
    dos33_recovery status = dos33_recovery::none;
    unsigned reused = 0;       /**< List + data sectors the VTOC map shows as used again. */
    unsigned lists = 0;        /**< T/S list sectors followed. */
    unsigned data_sectors = 0; /**< Data sectors copied. */
    std::string reason{};      /**< Why partial / unrecoverable (empty when full). */
    std::vector<uint8_t> bytes{}; /**< Recovered data, trimmed by the real-length rules. */
};

/**
 * @brief Recover a deleted DOS 3.3 file (`--undelete`).
 *
 * The old T/S list chain is checked with @ref dos33_ts_chain (range,
 * loop, at most @c DOS33_MAX_TS_LISTS lists) before it is read. Every
 * list and data sector is then looked up in the VTOC free-sector map
 * (offset $38, 4 bytes per track): DELETE frees them, so one that is
 * marked used again may hold a later file and makes the result partial.
 * A bad or looping chain, an out-of-range data pointer, or a volume that
 * is not 16 sectors per track (map layout not checked) is also partial.
 * Nothing is guessed: bytes are read as they are on the disk. I/A/B and
 * T lengths follow the same rules as live files.
 *
 * @param[in] disk Parsed DOS 3.3 volume.
 * @param[in] file Catalog entry; live entries return @c dos33_recovery::none.
 */
[[nodiscard]] dos33_undelete undelete_dos33(const apple_disk& disk, const apple_file& file);

/** @brief Short listing / JSON word: `full`, `partial`, `unrecoverable`, or empty. */
[[nodiscard]] const char* dos33_recovery_name(dos33_recovery r);

/**
 * @brief Host name for an `--undelete` extract: `NAME.undeleted` when
 *        full, `NAME.partial` when partial (`/` flattened to `_`).
 */
[[nodiscard]] std::string apple_undelete_filename(const apple_file& file, dos33_recovery r);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_APPLE_HPP */
