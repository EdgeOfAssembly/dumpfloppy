/**
 * @file analyze.hpp
 * @brief Full floppy-image analysis (BPB, FAT, directory, volume, boot).
 */
#ifndef DUMPFLOPPY_ANALYZE_HPP
#define DUMPFLOPPY_ANALYZE_HPP

#include "dumpfloppy/amiga.hpp"
#include "dumpfloppy/boot.hpp"
#include "dumpfloppy/catalog.hpp"
#include "dumpfloppy/cbm.hpp"
#include "dumpfloppy/fat.hpp"
#include "dumpfloppy/ibm_mfm.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/types.hpp"
#include "dumpfloppy/volume.hpp"

#include <span>
#include <string>
#include <vector>

namespace dumpfloppy
{

/**
 * @brief Complete dump of one image.
 *
 * Logical FAT bytes are @ref volume_bytes / @ref make_sector_store
 * (assembled IBM CHS when present, else raw @a image.bytes).
 * When @a cbm.present or @a amiga.present, FAT/HxC are not walked.
 */
struct analysis
{
    floppy_image image{};
    bpb_info bpb{};
    ebpb_info ebpb{};
    boot_info boot{};
    fat_kind kind = fat_kind::unknown;
    fat_summary fat{};
    volume_info volume{};
    std::vector<dir_entry> entries{};
    std::vector<uint16_t> orphan_clusters{};
    std::vector<std::string> secrets{};
    uint64_t volume_bytes = 0;
    uint64_t trailing_bytes = 0;
    bool truncated = false;
    flux_disk flux{};
    catalog_hit catalog{};
    cbm_disk cbm{};     /**< D64/D71/D81/G64 CBMFS; @a present is false on PC FAT / ADF. */
    amiga_disk amiga{}; /**< OFS/FFS ADF; @a present is false on PC FAT / CBM. */
};

/**
 * @brief Const view of the logical FAT volume bytes.
 *
 * If @a flux.assembled_chs is non-empty, that vector is the store
 * (512-byte IBM CHS). Else @a image.bytes (raw img/ima, or flux that
 * did not assemble CHS). Same selection as @ref make_ibm_store.
 *
 * @param[in] a Analysis after @ref analyse (or a synthetic test object).
 * @return Non-owning span into @p a; empty if both sources are empty.
 */
[[nodiscard]] std::span<const uint8_t> volume_bytes(const analysis& a);

/**
 * @brief Mutable logical FAT volume (same selection as @ref volume_bytes).
 *
 * Used by `-u` / @ref update_files to patch clusters in place. Callers
 * must not reseat the chosen vector (clearing @a assembled_chs after
 * taking this reference would dangle).
 *
 * @param[in,out] a Analysis whose CHS or raw bytes will be mutated.
 * @return Reference to @a assembled_chs or @a image.bytes.
 */
[[nodiscard]] std::vector<uint8_t>& volume_bytes_mut(analysis& a);

/**
 * @brief Named @ref sector_store wrapping @ref volume_bytes.
 *
 * Assembled IBM CHS is always @ref k_ibm_sector_bytes. Raw img/ima uses
 * @a bpb.bytes_per_sector when that field is non-zero, otherwise 512.
 *
 * @param[in] a Analysis to view.
 */
[[nodiscard]] sector_store make_sector_store(const analysis& a);

/**
 * @brief Analyse a loaded image.
 *
 * A D64/D71/D81 with a valid CBMFS BAM/header, a G64 with a valid
 * GCR-1541 header, or a DD/HD ADF with a valid OFS/FFS root, is parsed
 * as that filesystem; FAT/BPB and HxC flux decode are skipped so those
 * bytes are not treated as DOS. Catalog lookup still runs.
 *
 * @param[in] image Raw image from @ref load_image.
 */
[[nodiscard]] analysis analyse(floppy_image image);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_ANALYZE_HPP */
