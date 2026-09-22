/**
 * @file volume.hpp
 * @brief Logical FAT volume as a sector-addressable byte store.
 *
 * Seam between container/codec and the FAT filesystem: HxC IBM CHS when
 * assembled, otherwise the raw `.img` / `.ima` bytes. Not a D64/CBM parser.
 */
#ifndef DUMPFLOPPY_VOLUME_HPP
#define DUMPFLOPPY_VOLUME_HPP

#include "dumpfloppy/analyze.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace dumpfloppy
{

/** @brief IBM PC DAM / raw floppy sector size used for assembled CHS. */
inline constexpr uint32_t k_ibm_sector_bytes = 512u;

/**
 * @brief Sector-addressable logical volume (non-owning).
 *
 * @a bytes is the FAT image @ref analyse / extract / update walk:
 * 512-byte IBM CHS from @a flux.assembled_chs when that vector is
 * non-empty, otherwise @a image.bytes. @a sector_size is 512 for
 * assembled CHS; raw img/ima uses the BPB size when it is non-zero.
 */
struct sector_store
{
    std::span<const uint8_t> bytes{}; /**< Logical volume payload. */
    uint32_t sector_size = k_ibm_sector_bytes; /**< Bytes per sector. */
};

/**
 * @brief Const view of the logical FAT volume bytes.
 *
 * If @a flux.assembled_chs is non-empty, that vector is the store
 * (512-byte IBM CHS). Else @a image.bytes (raw img/ima, or flux that
 * did not assemble CHS). Same selection as the former inline ternary.
 *
 * @param[in] a Analysis after @ref analyse (or a synthetic test object).
 * @return Non-owning span into @p a; empty if both sources are empty.
 */
[[nodiscard]] inline std::span<const uint8_t> volume_bytes(const analysis& a)
{
    if (!a.flux.assembled_chs.empty())
    {
        return a.flux.assembled_chs;
    }
    return a.image.bytes;
}

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
[[nodiscard]] inline std::vector<uint8_t>& volume_bytes_mut(analysis& a)
{
    if (!a.flux.assembled_chs.empty())
    {
        return a.flux.assembled_chs;
    }
    return a.image.bytes;
}

/**
 * @brief Named @ref sector_store wrapping @ref volume_bytes.
 *
 * Assembled IBM CHS is always @ref k_ibm_sector_bytes. Raw img/ima uses
 * @a bpb.bytes_per_sector when that field is non-zero, otherwise 512.
 *
 * @param[in] a Analysis to view.
 */
[[nodiscard]] inline sector_store make_sector_store(const analysis& a)
{
    const uint32_t ss = !a.flux.assembled_chs.empty()
                            ? k_ibm_sector_bytes
                            : (a.bpb.bytes_per_sector != 0u ? a.bpb.bytes_per_sector
                                                            : k_ibm_sector_bytes);
    return sector_store{.bytes = volume_bytes(a), .sector_size = ss};
}

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_VOLUME_HPP */
