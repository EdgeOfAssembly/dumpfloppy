/**
 * @file report.hpp
 * @brief Human-readable dump (ANSI colour; deleted entries on light red).
 */
#ifndef DUMPFLOPPY_REPORT_HPP
#define DUMPFLOPPY_REPORT_HPP

#include "dumpfloppy/analyze.hpp"

#include <iosfwd>

namespace dumpfloppy
{

/** @brief Rendering switches for a text report. */
struct report_options
{
    bool color = true;         /**< ANSI; deleted = light-red bg + white bold. */
    bool hex_boot = true;      /**< 512-byte boot-sector hex dump. */
    bool show_deleted = true;  /**< Include 0xE5 directory slots. */
};

/**
 * @brief Write a full analysis to @p out.
 *
 * @param[in]  a    Analysis from @ref analyse.
 * @param[out] out  Destination stream (stdout or a file).
 * @param[in]  opt  Colour / hex / deleted switches.
 */
void write_report(const analysis& a, std::ostream& out, const report_options& opt);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_REPORT_HPP */
