/**
 * @file volume.hpp
 * @brief Logical IBM PC volume as a sector-addressable byte store.
 *
 * Seam between container/codec and the FAT filesystem: assembled HxC IBM
 * CHS when non-empty, otherwise the raw `.img` / `.ima` bytes. This header
 * does not include @c analyze.hpp; analysis adapters live next to
 * @ref analyse. Not a D64/CBM parser.
 */
#ifndef DUMPFLOPPY_VOLUME_HPP
#define DUMPFLOPPY_VOLUME_HPP

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
 * 512-byte IBM CHS when that span is non-empty, otherwise raw image
 * bytes. @a sector_size is 512 for assembled CHS; raw img/ima uses the
 * BPB size when it is non-zero.
 */
struct sector_store
{
    std::span<const uint8_t> bytes{}; /**< Logical volume payload. */
    uint32_t sector_size = k_ibm_sector_bytes; /**< Bytes per sector. */
};

/**
 * @brief Select IBM CHS bytes if non-empty, else raw image bytes.
 *
 * Assembled IBM CHS is always @ref k_ibm_sector_bytes. Raw img/ima uses
 * @p bpb_bps when that value is non-zero, otherwise 512.
 *
 * @param[in] assembled_chs 512-byte IBM CHS from flux; empty → use @p raw.
 * @param[in] raw           `.img` / `.ima` (or unassembled flux) bytes.
 * @param[in] bpb_bps       BPB bytes/sector; ignored when CHS is selected.
 * @return Non-owning store; empty if both sources are empty.
 */
[[nodiscard]] inline sector_store make_ibm_store(
    std::span<const uint8_t> assembled_chs, std::span<const uint8_t> raw,
    uint32_t bpb_bps)
{
    if (!assembled_chs.empty())
    {
        return sector_store{.bytes = assembled_chs,
                            .sector_size = k_ibm_sector_bytes};
    }
    const uint32_t ss = (bpb_bps != 0u) ? bpb_bps : k_ibm_sector_bytes;
    return sector_store{.bytes = raw, .sector_size = ss};
}

/**
 * @brief Mutable IBM volume (same CHS-vs-raw rule as @ref make_ibm_store).
 *
 * @param[in,out] assembled_chs Flux CHS vector; used when non-empty.
 * @param[in,out] raw           Image bytes used when @p assembled_chs is empty.
 * @return Reference to the selected vector.
 *
 * @warning Callers must not reseat the chosen vector (clearing CHS after
 *          taking this reference would dangle).
 */
[[nodiscard]] inline std::vector<uint8_t>& ibm_volume_mut(
    std::vector<uint8_t>& assembled_chs, std::vector<uint8_t>& raw)
{
    if (!assembled_chs.empty())
    {
        return assembled_chs;
    }
    return raw;
}

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_VOLUME_HPP */
