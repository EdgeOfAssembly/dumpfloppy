/**
 * @file extract.hpp
 * @brief Write recovered FAT files to the host filesystem.
 */
#ifndef DUMPFLOPPY_EXTRACT_HPP
#define DUMPFLOPPY_EXTRACT_HPP

#include "dumpfloppy/analyze.hpp"

#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace dumpfloppy
{

/** @brief Extract switch: off by default; empty patterns means every file. */
struct extract_options
{
    bool enabled = false;
    std::vector<std::string> patterns{}; /**< DOS globs; empty → all payloads. */
    std::filesystem::path dest_dir{"."};
};

/**
 * @brief True if @p e should be extracted under @p opt.
 *
 * Matches @a name_83, @a path, and @a lfn case-insensitively against each
 * glob. Directories, volume labels, `.` and `..` are never extracted.
 */
[[nodiscard]] bool extract_matches(const dir_entry& e, const extract_options& opt);

/**
 * @brief Write matching files into @p opt.dest_dir.
 *
 * @param[in]  a   Analysis with cluster chains.
 * @param[in]  opt Extract flags and destination.
 * @param[out] err Diagnostics (typically stderr).
 *
 * @return Number of files written, or -1 if a write failed or a pattern
 *         matched nothing.
 */
[[nodiscard]] int extract_files(const analysis& a, const extract_options& opt,
                                std::ostream& err);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_EXTRACT_HPP */
