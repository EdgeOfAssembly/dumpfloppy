/**
 * @file foreign.hpp
 * @brief Parsers for SPS IPF, Apple WOZ, Pasti STX, and Apple 2IMG.
 *
 * These containers are flux or prefixed nibble dumps. @ref analyse stores
 * the result in @c analysis::foreign and skips FAT when @a present is true.
 */
#ifndef DUMPFLOPPY_FOREIGN_HPP
#define DUMPFLOPPY_FOREIGN_HPP

#include "dumpfloppy/flux_view.hpp"
#include "dumpfloppy/foreign_view.hpp"

#include <cstdint>
#include <span>

namespace dumpfloppy
{

/**
 * @brief Parse IPF, WOZ, STX, or 2IMG. First match wins.
 *
 * @param[in] data Whole image.
 */
[[nodiscard]] foreign_disk parse_foreign(std::span<const uint8_t> data);

/**
 * @brief Assemble standard 512-byte STX sectors into IBM CHS.
 *
 * Copy-protected / fuzzy / non-512 sectors are recorded on @a flux.protection
 * and omitted from @a assembled_chs. Empty @a assembled_chs means no GEMDOS
 * volume could be built.
 *
 * @param[in] data Whole STX file (including the 16-byte Pasti header).
 */
[[nodiscard]] flux_disk assemble_stx(std::span<const uint8_t> data);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_FOREIGN_HPP */
