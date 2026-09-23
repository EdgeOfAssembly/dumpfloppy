/**
 * @file g64.hpp
 * @brief Commodore G64 (GCR-1541) container: decode tracks to a 35-track D64.
 *
 * Magic `GCR-1541`, version 0, then a track-offset table (usually 84 half-
 * tracks). Whole tracks 1–35 are GCR-decoded into 256-byte CBM sectors.
 * @ref parse_g64 fills @ref cbm_disk::decoded and sets @a present when the
 * header is usable so @ref analyse skips FAT even if CBMFS is unreadable.
 */
#ifndef DUMPFLOPPY_G64_HPP
#define DUMPFLOPPY_G64_HPP

#include "dumpfloppy/cbm.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace dumpfloppy
{

/** @brief 8-byte G64 signature. */
inline constexpr char k_g64_magic[] = "GCR-1541";

/** @brief Only defined G64 version. */
inline constexpr uint8_t k_g64_version = 0u;

/** @brief Usual table length: 42 tracks × 2 (whole + half). */
inline constexpr uint8_t k_g64_usual_tracks = 84u;

/** @brief Maximum accepted track-table length (1541 half-track map). */
inline constexpr uint8_t k_g64_max_tracks = 84u;

/** @brief Typical per-track allocation in the container (bytes). */
inline constexpr uint16_t k_g64_usual_track_bytes = 7928u;

/** @brief Header prefix before the offset table (`GCR-1541` + ver + n + max). */
inline constexpr std::size_t k_g64_prefix_bytes = 12u;

/**
 * @brief True when @p image starts with `GCR-1541` version 0 and a usable table.
 *
 * @param[in] image Whole G64 (or a prefix long enough for the header).
 */
[[nodiscard]] bool is_g64_image(std::span<const uint8_t> image);

/**
 * @brief Decode whole tracks 1–35 into a 174848-byte D64 sector image.
 *
 * @param[in] image Whole G64.
 *
 * @return 174848 bytes (missing sectors stay 0), or empty when the header
 *         is not a G64.
 */
[[nodiscard]] std::vector<uint8_t> g64_decode_d64(std::span<const uint8_t> image);

/**
 * @brief Parse a G64: decode GCR, then CBMFS on the 1541 sector map.
 *
 * @param[in] image Whole image.
 *
 * @return @a present true when the G64 header is valid. @a media is
 *         @ref cbm_media::g64. @a decoded holds the D64 map. Directory
 *         fields are filled only when BAM at 18/0 looks like CBMFS.
 */
[[nodiscard]] cbm_disk parse_g64(std::span<const uint8_t> image);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_G64_HPP */
