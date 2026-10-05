/**
 * @file cpm.hpp
 * @brief CP/M 2.2 directory listing (names only; no file extractor).
 */
#ifndef DUMPFLOPPY_CPM_HPP
#define DUMPFLOPPY_CPM_HPP

#include "dumpfloppy/types.hpp"

#include <span>
#include <vector>

namespace dumpfloppy
{

/**
 * @brief List CP/M 2.2 directory names.
 *
 * A candidate slot is 32-byte aligned. The user byte is `0x00`–`0x0F` (live)
 * or `0xE5` (deleted). The 11 name bytes are space, digit, or `A`–`Z`. The
 * reserved byte at offset 13 is 0. A hit is a run of at least two such slots.
 * Extents of one name collapse to a single @ref dir_entry. Deleted slots stay
 * deleted and are not renamed into a live file.
 *
 * @param[in] image Whole sector image.
 * @return Live and deleted names. Empty when no run matches.
 */
[[nodiscard]] std::vector<dir_entry> list_cpm_directory(std::span<const uint8_t> image);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_CPM_HPP */
