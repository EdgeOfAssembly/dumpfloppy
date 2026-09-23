/**
 * @file geometry.hpp
 * @brief Floppy size → CHS table and media-descriptor names.
 */
#ifndef DUMPFLOPPY_GEOMETRY_HPP
#define DUMPFLOPPY_GEOMETRY_HPP

#include "dumpfloppy/types.hpp"

#include <cstdint>
#include <string>

namespace dumpfloppy
{

/**
 * @brief Guess CHS from a raw image length (standard IBM PC formats).
 *
 * @param[in] byte_count File size in bytes.
 * @return Filled geometry; @a expected_bytes is 0 when the size is non-standard.
 */
[[nodiscard]] geometry geometry_from_size(uint64_t byte_count);

/**
 * @brief Human name for a BPB media descriptor byte.
 *
 * @param[in] media Media ID (FAT[0] low byte / BPB byte 0x15).
 */
[[nodiscard]] std::string media_descriptor_name(uint8_t media);

/**
 * @brief Guess container from the path extension (`.img` / `.ima` / `.d64` /
 *        `.d71` / `.d81` / `.adf` / `.g64`).
 */
[[nodiscard]] container_kind container_from_path(const std::string& path);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_GEOMETRY_HPP */
