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

/** @brief Rendering switches for a text report. */
struct report_options
{
    bool color = true;         /**< ANSI; deleted = light-red bg + white bold. */
    bool hex_boot = true;      /**< 512-byte boot-sector hex dump. */
    bool show_deleted = true;  /**< Include 0xE5 directory slots. */
    bool show_unused = true;   /**< List leftover FAT-free/bad cluster runs. */
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

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_REPORT_HPP */
