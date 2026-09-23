/**
 * @file g71.hpp
 * @brief Commodore G71 (GCR-1571) container: decode tracks to a 70-track D71.
 *
 * Magic `GCR-1571`, version 0, then a track-offset table. VICE stores 168
 * half-tracks (side 0 = slots 0–83, side 1 = 84–167) or 84 whole tracks
 * (side 0 = 1–42, side 1 = 43–84). GCR header T/S is the 1571 logical
 * track (1–70). @ref parse_g71 fills @ref cbm_disk::decoded and sets
 * @a present when the header is usable so @ref analyse skips FAT even if
 * CBMFS is unreadable.
 */
#ifndef DUMPFLOPPY_G71_HPP
#define DUMPFLOPPY_G71_HPP

#include "dumpfloppy/cbm.hpp"
#include "dumpfloppy/g64.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace dumpfloppy
{

/** @brief 8-byte G71 signature. */
inline constexpr char k_g71_magic[] = "GCR-1571";

/** @brief Only defined G71 version (same as G64). */
inline constexpr uint8_t k_g71_version = 0u;

/** @brief Usual table length: 42 tracks × 2 sides × 2 (whole + half). */
inline constexpr uint8_t k_g71_usual_tracks = 168u;

/** @brief Maximum accepted track-table length (1571 half-track map). */
inline constexpr uint8_t k_g71_max_tracks = 168u;

/** @brief Typical per-track allocation (same as G64). */
inline constexpr uint16_t k_g71_usual_track_bytes = k_g64_usual_track_bytes;

/** @brief Header prefix before the offset table (`GCR-1571` + ver + n + max). */
inline constexpr std::size_t k_g71_prefix_bytes = k_g64_prefix_bytes;

/**
 * @brief True when @p image starts with `GCR-1571` version 0 and a usable table.
 *
 * @param[in] image Whole G71 (or a prefix long enough for the header).
 */
[[nodiscard]] bool is_g71_image(std::span<const uint8_t> image);

/**
 * @brief Decode whole tracks into a 349696-byte D71 sector image.
 *
 * Sectors are stored by GCR header T/S using 1571 geometry (tracks 1–35
 * side 0, 36–70 side 1). Half-track slots are skipped when the table has
 * more than 84 entries.
 *
 * @param[in] image Whole G71.
 *
 * @return 349696 bytes (missing sectors stay 0), or empty when the header
 *         is not a G71.
 */
[[nodiscard]] std::vector<uint8_t> g71_decode_d71(std::span<const uint8_t> image);

/**
 * @brief Parse a G71: decode GCR, then CBMFS on the 1571 sector map.
 *
 * @param[in] image Whole image.
 *
 * @return @a present true when the G71 header is valid. @a media is
 *         @ref cbm_media::g71. @a decoded holds the D71 map. Directory
 *         fields are filled only when BAM at 18/0 looks like CBMFS.
 */
[[nodiscard]] cbm_disk parse_g71(std::span<const uint8_t> image);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_G71_HPP */
