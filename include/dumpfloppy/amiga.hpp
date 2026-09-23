/**
 * @file amiga.hpp
 * @brief Amiga OFS/FFS ADF geometry, directory walk, and file extract.
 *
 * DD (80×2×11×512) and HD (80×2×22×512) raw ADF images. Bootblock `DOS`
 * plus type 0–5 (even OFS, odd FFS). Rootblock at sector_count/2.
 * @ref analyse stores the result in @c analysis::amiga and skips FAT when
 * @a present is true.
 */
#ifndef DUMPFLOPPY_AMIGA_HPP
#define DUMPFLOPPY_AMIGA_HPP

#include "dumpfloppy/amiga_view.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace dumpfloppy
{

/** @brief Bytes per ADF sector (OFS header + FFS raw block). */
inline constexpr uint32_t k_adf_sector_bytes = 512u;

/** @brief Hash-table / data-pointer slots in a 512-byte header. */
inline constexpr uint32_t k_adf_ht_size = 72u;

/** @brief OFS data-block payload after the 24-byte header. */
inline constexpr uint32_t k_ofs_data_payload = 488u;

/** @brief Maximum BCPL name length (length byte + 30 chars). */
inline constexpr uint32_t k_amiga_name_max = 30u;

/** @brief DD ADF: 80 cyl × 2 heads × 11 sectors × 512. */
inline constexpr std::size_t k_adf_dd_bytes = 901120u;

/** @brief HD ADF: 80 cyl × 2 heads × 22 sectors × 512. */
inline constexpr std::size_t k_adf_hd_bytes = 1802240u;

/** @brief DD sector count (root at 880). */
inline constexpr uint32_t k_adf_dd_sectors = 1760u;

/** @brief HD sector count (root at 1760). */
inline constexpr uint32_t k_adf_hd_sectors = 3520u;

/** @brief File-header secondary type (big-endian int32). */
inline constexpr int32_t k_amiga_st_file = -3;

/** @brief User-directory secondary type. */
inline constexpr int32_t k_amiga_st_userdir = 2;

/** @brief Rootblock secondary type. */
inline constexpr int32_t k_amiga_st_root = 1;

/**
 * @brief True for a 901120-byte DD ADF.
 *
 * @param[in] n Image length in bytes.
 */
[[nodiscard]] constexpr bool is_adf_dd_size(std::size_t n) noexcept
{
    return n == k_adf_dd_bytes;
}

/**
 * @brief True for a 1802240-byte HD ADF.
 *
 * @param[in] n Image length in bytes.
 */
[[nodiscard]] constexpr bool is_adf_hd_size(std::size_t n) noexcept
{
    return n == k_adf_hd_bytes;
}

/**
 * @brief True for DD or HD ADF size.
 *
 * @param[in] n Image length in bytes.
 */
[[nodiscard]] constexpr bool is_adf_size(std::size_t n) noexcept
{
    return is_adf_dd_size(n) || is_adf_hd_size(n);
}

/**
 * @brief Sector count for a DD/HD ADF size, or 0.
 *
 * @param[in] n Image length in bytes.
 */
[[nodiscard]] constexpr uint32_t adf_sector_count_for_size(std::size_t n) noexcept
{
    if (is_adf_dd_size(n))
    {
        return k_adf_dd_sectors;
    }
    if (is_adf_hd_size(n))
    {
        return k_adf_hd_sectors;
    }
    return 0u;
}

/**
 * @brief Rootblock sector (sector_count / 2).
 *
 * @param[in] sector_count DD 1760 or HD 3520.
 */
[[nodiscard]] constexpr uint32_t adf_root_block(uint32_t sector_count) noexcept
{
    return sector_count / 2u;
}

/**
 * @brief True when the DOS type byte is odd (FFS / FFS-Intl / FFS-DirCache).
 *
 * @param[in] dos_type Bootblock byte 3.
 */
[[nodiscard]] constexpr bool amiga_dos_is_ffs(uint8_t dos_type) noexcept
{
    return (dos_type & 1u) != 0u;
}

/**
 * @brief `"OFS"` or `"FFS"` for listings.
 *
 * @param[in] ffs True for odd DOS types.
 */
[[nodiscard]] inline const char* amiga_fs_name(bool ffs) noexcept
{
    return ffs ? "FFS" : "OFS";
}

/**
 * @brief Host basename: path with `/` and other unsafe bytes mapped to `_`.
 *
 * @param[in] file Directory slot (uses @a path, else @a name).
 */
[[nodiscard]] std::string amiga_host_filename(const amiga_file& file);

/**
 * @brief Parse an ADF bootblock + OFS/FFS root and walk the hash trees.
 *
 * @param[in] image Whole DD/HD ADF.
 *
 * @return @a present false unless size is DD/HD, boot is `DOS` + 0–5, and
 *         the rootblock is type 2 / ST_ROOT / ht_size 72.
 *
 * @note Recurses ST_USERDIR with a visited-block guard. Hash chains at
 *       offset 496. File size at 0x144. Does not verify block checksums.
 */
[[nodiscard]] amiga_disk parse_adf(std::span<const uint8_t> image);

/**
 * @brief Read a file payload from OFS (24-byte data headers) or FFS (raw 512).
 *
 * Data keys live at `24 + (72-1-i)*4` for `i` in `0 .. high_seq-1`. Further
 * keys follow the extension block at offset 504 (T_LIST). Output is trimmed
 * to the on-disk byte size.
 *
 * @param[in] image Whole ADF.
 * @param[in] disk  Parsed disk (`ffs` / `sector_count`).
 * @param[in] file  File header from @ref parse_adf.
 *
 * @return Payload bytes (empty for directories, block 0, or unreadable).
 */
[[nodiscard]] std::vector<uint8_t> read_amiga_file(std::span<const uint8_t> image,
                                                   const amiga_disk& disk,
                                                   const amiga_file& file);

/**
 * @brief Overwrite OFS/FFS data blocks in place when @p payload matches @a byte_size.
 *
 * OFS data-block checksums at offset 20 are recomputed. Header size and
 * block pointers are left unchanged. Directories are rejected.
 *
 * @param[in,out] image   DD/HD ADF bytes.
 * @param[in]     disk    Parsed disk (`ffs` / `sector_count`).
 * @param[in]     file    File header from @ref parse_adf.
 * @param[in]     payload Host bytes; length must equal @a file.byte_size.
 * @param[out]    err     Reason on failure.
 *
 * @retval true  Payload written.
 * @retval false Size mismatch, missing block, or directory; @p err set.
 */
[[nodiscard]] bool write_amiga_file_same_size(std::vector<uint8_t>& image,
                                              const amiga_disk& disk,
                                              const amiga_file& file,
                                              std::span<const uint8_t> payload,
                                              std::string& err);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_AMIGA_HPP */
