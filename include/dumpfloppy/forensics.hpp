/**
 * @file forensics.hpp
 * @brief Optional FAT12/FAT16 slack, leaked-directory, and carve scans.
 */
#ifndef DUMPFLOPPY_FORENSICS_HPP
#define DUMPFLOPPY_FORENSICS_HPP

#include "dumpfloppy/analyze.hpp"

#include <iosfwd>

namespace dumpfloppy
{

/** @brief Enable-only forensic scans. All default off. */
struct forensics_request
{
    bool slack = false;  /**< Unused tail of each live payload file. */
    bool leaked = false; /**< Directory slots outside walked tables. */
    bool carve = false;  /**< MZ/ZM, GIF, and long ASCII in slack and free space. */
};

/**
 * @brief Print the requested forensic sections.
 *
 * FAT12 and FAT16 only. Any other filesystem writes
 * `dumpfloppy: slack is only implemented for FAT12/FAT16` (and the leaked
 * and carve lines for whichever flags are set) to @p err and returns 1
 * without a section. Slack length comes from @ref fat_slack_bytes.
 *
 * Stdout sections, when that flag is set:
 * - `=== Slack ===` — path, image offset, length, hex of the tail.
 * - `=== Leaked directory entries ===` — offset, escaped name, attr,
 *   cluster, size. Slots are 32-byte aligned to the cluster's file
 *   offset (or to volume byte 0 past the filesystem end). A window that
 *   starts before the region or extends past it is not a hit. At most
 *   64 hits.
 * - `=== Carve ===` — offset, kind, length, and at most 48 characters.
 *   At most 64 hits.
 *
 * A 65th hit writes one stderr warning and stops that scan.
 *
 * @param[in]  a   Analysis of one image.
 * @param[in]  req Which scans to run.
 * @param[out] out Section text (typically stdout).
 * @param[out] err Cap warnings and unsupported-filesystem lines.
 *
 * @retval 0 Scan finished (possibly with a cap warning).
 * @retval 1 Filesystem is not FAT12 or FAT16.
 */
[[nodiscard]] int write_forensics(const analysis& a, const forensics_request& req,
                                  std::ostream& out, std::ostream& err);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_FORENSICS_HPP */
