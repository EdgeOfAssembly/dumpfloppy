/**
 * @file trd.hpp
 * @brief ZX Spectrum TR-DOS TRD parser (256-byte sectors, 16 per track).
 *
 * Disk-info lives at logical sector 8. Size alone is not enough: 160K/320K
 * collide with IBM PC images, so @ref is_trd_image requires the 0x10 ID or
 * a plausible geometry stamp. @ref analyse stores the result in
 * @c analysis::trd and skips FAT when @a present is true.
 */
#ifndef DUMPFLOPPY_TRD_HPP
#define DUMPFLOPPY_TRD_HPP

#include "dumpfloppy/trd_view.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace dumpfloppy
{

/** @brief Data bytes per TR-DOS sector. */
inline constexpr uint32_t k_trd_sector_bytes = 256u;

/** @brief Sectors per logical track. */
inline constexpr uint8_t k_trd_spt = 16u;

/** @brief Directory occupies logical sectors 0–7 (128 × 16-byte slots). */
inline constexpr std::size_t k_trd_dir_slots = 128u;

/** @brief Disk-info logical sector (track 0, sector 8). */
inline constexpr std::size_t k_trd_info_sector = 8u;

/** @brief 40-track SS TRD (160K). */
inline constexpr std::size_t k_trd_ss40_bytes = 163840u;

/** @brief 40-track DS or 80-track SS TRD (320K). */
inline constexpr std::size_t k_trd_320_bytes = 327680u;

/** @brief 80-track DS TRD (640K). */
inline constexpr std::size_t k_trd_ds80_bytes = 655360u;

/**
 * @brief True when @p data is a TRD with a TR-DOS disk-info stamp.
 *
 * @param[in] data Whole image.
 */
[[nodiscard]] bool is_trd_image(std::span<const uint8_t> data);

/**
 * @brief Parse directory and disk-info. @a present is false on failure.
 *
 * @param[in] data Whole TRD image.
 */
[[nodiscard]] trd_disk parse_trd(std::span<const uint8_t> data);

/**
 * @brief Host extract name (`HELLO.C`; deleted `?ELLO.C`).
 *
 * @param[in] file Directory slot.
 */
[[nodiscard]] std::string trd_host_filename(const trd_file& file);

/**
 * @brief Read the file's allocated sectors, trimmed to @a byte_size when smaller.
 *
 * @param[in] image Whole TRD bytes.
 * @param[in] file  Directory slot.
 */
[[nodiscard]] std::vector<uint8_t> read_trd_file(std::span<const uint8_t> image,
                                                 const trd_file& file);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_TRD_HPP */
