/**
 * @file report.hpp
 * @brief Human-readable dump (ANSI colour; deleted entries on light red).
 */
#ifndef DUMPFLOPPY_REPORT_HPP
#define DUMPFLOPPY_REPORT_HPP

#include <iosfwd>
#include <string>

namespace dumpfloppy
{

struct analysis; /**< Complete type in analyze.hpp. */
struct dir_entry; /**< Complete type in types.hpp. */

/** @brief Rendering switches for a text or JSON report. */
struct report_options
{
    bool color = true;         /**< ANSI; deleted = light-red bg + white bold. */
    bool hex_boot = true;      /**< 512-byte boot-sector hex dump. */
    bool show_deleted = true;  /**< Include 0xE5 directory slots. */
    bool show_unused = true;   /**< List leftover FAT-free/bad cluster runs. */
    bool json = false;         /**< Enable-only `--json` (one JSON document). */
};

/**
 * @brief Write a full analysis to @p out.
 *
 * @param[in]  a    Analysis from @ref analyse.
 * @param[out] out  Destination stream (stdout or a file).
 * @param[in]  opt  Colour / hex / deleted switches.
 */
void write_report(const analysis& a, std::ostream& out, const report_options& opt);

/**
 * @brief Format one FAT directory row the listing prints.
 *
 * The mark is a blank when @p e is not deleted. The size field is ten
 * digits wide, the same width as the Size header, so `4294967295` is
 * not clipped. CBM, Amiga, TRD, and Apple listings share that width.
 *
 * @param[in] e FAT directory entry (live or deleted).
 * @return The row text, without a trailing newline.
 */
[[nodiscard]] std::string entry_line(const dir_entry& e);

/**
 * @brief Open a JSON report (`tool`, `version`, and the `images` array).
 *
 * The document is plain JSON. No ANSI, no hex dump, and no secrets.
 * Pair with @ref write_json_image and @ref write_json_end on the same stream.
 *
 * @param[out] out Destination (stdout or a file).
 */
void write_json_begin(std::ostream& out);

/**
 * @brief Append one image object to a document opened by @ref write_json_begin.
 *
 * `filesystem` is the family name the text report already uses (`FAT12`,
 * `FAT16`, `CBM`, `Amiga`, `TRD`, `Apple`, or the foreign format).
 * `xxh64` is the hash already stored on a FAT entry, or an empty string.
 * This function does not hash payload bytes. @a opt.show_deleted filters
 * entries the same way as the text listing.
 *
 * @param[in]  a     Analysis from @ref analyse.
 * @param[out] out   Same stream passed to @ref write_json_begin.
 * @param[in]  opt   Listing switches. Colour and hex are ignored.
 * @param[in]  first True for the first image so no comma is written before it.
 */
void write_json_image(const analysis& a, std::ostream& out, const report_options& opt,
                      bool first);

/**
 * @brief Close a document opened by @ref write_json_begin.
 *
 * @param[out] out Same stream.
 */
void write_json_end(std::ostream& out);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_REPORT_HPP */
