/**
 * @file update.hpp
 * @brief Replace a named FAT file inside a floppy volume (`-u` / `--update`).
 */
#ifndef DUMPFLOPPY_UPDATE_HPP
#define DUMPFLOPPY_UPDATE_HPP

#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace dumpfloppy
{

struct analysis; /**< Complete type in analyze.hpp. */

/** @brief Update switch: off by default; each host path replaces its 8.3 name. */
struct update_options
{
    bool enabled = false;
    std::vector<std::filesystem::path> hosts{}; /**< Host files; basename is the 8.3 key. */
};

/**
 * @brief Overwrite live FAT files whose 8.3 name matches each host basename.
 *
 * Same size: payload is written onto the existing cluster chain. Smaller:
 * the tail of the chain is freed. Larger: free clusters are allocated
 * (fragmented if needed). When the next sequential clusters belong to
 * another live file, that file is relocated so this one can grow. A failed
 * relocate aborts the replace so @ref update_files can restore the volume
 * snapshot — the neighbour is never left with a freed chain and a stale
 * dirent. Deleted dirents are reclaimed only when their clusters are still
 * FAT-allocated and not present in any live file or directory chain (a
 * deleted TACTICS.PKG must not zero the live package's FAT).
 *
 * Mutates the FAT volume in @p a via @ref volume_bytes_mut (`assembled_chs`
 * when present, otherwise @a image.bytes). Directory size / first-cluster
 * and both FAT copies are kept in sync. Deleted names are never chosen.
 * D64/D71/D81 CBMFS and Amiga ADF images are refused in this version.
 *
 * @param[in,out] a   Analysis (volume bytes and directory entries).
 * @param[in]     opt Host files to write in.
 * @param[out]    err Diagnostics (typically stderr).
 *
 * @return Number of files replaced, or -1 on error (volume restored).
 */
[[nodiscard]] int update_files(analysis& a, const update_options& opt,
                               std::ostream& err);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_UPDATE_HPP */
