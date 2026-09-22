/**
 * @file analyze.hpp
 * @brief Full floppy-image analysis (BPB, FAT, directory, volume, boot).
 */
#ifndef DUMPFLOPPY_ANALYZE_HPP
#define DUMPFLOPPY_ANALYZE_HPP

#include "dumpfloppy/boot.hpp"
#include "dumpfloppy/catalog.hpp"
#include "dumpfloppy/fat.hpp"
#include "dumpfloppy/ibm_mfm.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/types.hpp"

#include <string>
#include <vector>

namespace dumpfloppy
{

/** @brief Complete dump of one image. */
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
};

/**
 * @brief Analyse a loaded image.
 *
 * @param[in] image Raw image from @ref load_image.
 */
[[nodiscard]] analysis analyse(floppy_image image);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_ANALYZE_HPP */
