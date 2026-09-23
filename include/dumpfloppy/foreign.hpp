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
#include <vector>

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

/**
 * @brief Decode WOZ 5.25 6-and-2 tracks into a DOS-order 140K/160K image.
 *
 * Integer TMAP slots 0–39 are scanned for `D5 AA 96` / `D5 AA AD`. Address
 * field T/S is stored as DOS 3.3 order (track×16+sector)×256. 3.5-inch WOZ
 * and empty TMAP yield an empty vector.
 *
 * @param[in] data Whole WOZ1/WOZ2 file.
 */
[[nodiscard]] std::vector<uint8_t> assemble_woz(std::span<const uint8_t> data);

/**
 * @brief Decode standard AmigaDOS sectors from an SPS IPF into a DD/HD ADF.
 *
 * IMGE/DATA tracks are expanded to MFM cells (CAPS encoder types 1–2, gap
 * type 0). `4489` syncs are Amiga even/odd decoded. Copy-protected tracks
 * that do not yield `0xFF` sectors are skipped. Empty if no sector decoded.
 *
 * @param[in] data Whole IPF file.
 */
[[nodiscard]] std::vector<uint8_t> assemble_ipf(std::span<const uint8_t> data);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_FOREIGN_HPP */
