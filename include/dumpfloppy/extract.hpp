/**
 * @file extract.hpp
 * @brief Write recovered FAT files to the host filesystem.
 */
#ifndef DUMPFLOPPY_EXTRACT_HPP
#define DUMPFLOPPY_EXTRACT_HPP

#include "dumpfloppy/types.hpp"

#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace dumpfloppy
{

struct analysis; /**< Complete type in analyze.hpp. */

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
 * CBMFS D64/D71/D81/G64 images extract PETSCII names plus `.prg`/`.seq`/`.usr`/
 * `.rel`/`.del` (deleted slots included). ADF images extract OFS/FFS files
 * (directories skipped) using @ref amiga_host_filename (`/` flattened to
 * `_`). FAT images walk cluster chains as before.
 *
 * Unsafe names (absolute paths, root names, empty components, `.`, `..`)
 * are skipped with a diagnostic — they must not escape @p opt.dest_dir.
 * If two payloads map to the same host path, the later file is written as
 * the 8.3 name (deleted entries keep `?`) or `stem.deleted.ext`; a warning
 * is emitted and the earlier file is left intact.
 *
 * @param[in]  a   Analysis with cluster chains.
 * @param[in]  opt Extract flags and destination.
 * @param[out] err Diagnostics (typically stderr).
 *
 * @return Number of files written, or -1 on failure.
 * @retval >=0 Files written (unsafe names are skipped, not counted).
 * @retval -1  Write failed, a pattern matched nothing, or the image is
 *             86Box `.86f` flux (sector map needs HxC `.mfm`).
 */
[[nodiscard]] int extract_files(const analysis& a, const extract_options& opt,
                                std::ostream& err);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_EXTRACT_HPP */
