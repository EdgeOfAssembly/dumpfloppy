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
 * `.rel`/`.del` (deleted slots included). FAT leftover free/bad clusters
 * extract as `unused_cNNNN.{c,map,txt,bin}`. ADF images extract OFS/FFS files
 * (directories skipped) using @ref amiga_host_filename (`/` flattened to
 * `_`). TRD images extract TR-DOS files as `NAME.C` (deleted `?AME.C`).
 * Apple DOS 3.3 / ProDOS (raw or 2IMG) extract catalog names. STX images
 * with assembled 512-byte sectors extract GEMDOS/FAT files. FAT images
 * walk cluster chains as before.
 *
 * Unsafe names (absolute paths, root names, empty components, `.`, `..`)
 * are skipped with a diagnostic — they must not escape @p opt.dest_dir.
 * After each filesystem's host-name function returns, bytes outside
 * printable ASCII (0x20–0x7E) become `_`. A printable deleted-entry `?`
 * (0x3F) is kept.
 * FAT keeps `/` (a `\` from the directory walk is rewritten to `/`) so
 * `RAMTEST/MANUAL.RT` is created under that subdirectory. A FAT component
 * longer than 255 bytes is shortened to a 240-byte prefix, `_`, and 8
 * lowercase hex digits of FNV-1a of that component.
 * Amiga, CBM, TRD, and Apple paths stay flattened (`/` and `\` become
 * `_`) and do not create subdirectories.
 * If two payloads map to the same host path, the later file is written as
 * the 8.3 name (deleted entries keep `?`, not `.dup`) or `stem.dup.ext`
 * (`FOO.dup.TXT`, then `FOO.dup.2.TXT`). A warning names the path written.
 * The earlier file is left intact.
 * A destination component that is a symlink is not followed. The file is
 * skipped (`dumpfloppy: skip symlink path '...'`) and the link target is
 * not created or truncated. Missing parents are created with mkdir.
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
