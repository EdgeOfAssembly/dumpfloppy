/**
 * @file ibm_mfm.hpp
 * @brief IBM MFM sector map and copy-protection heuristics (HxC .mfm).
 */
#ifndef DUMPFLOPPY_IBM_MFM_HPP
#define DUMPFLOPPY_IBM_MFM_HPP

#include "dumpfloppy/flux_view.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace dumpfloppy
{

/**
 * @brief Parse an HxC `.mfm` bitstream image into IBM sectors.
 */
[[nodiscard]] flux_disk decode_hxc_mfm(std::span<const uint8_t> file);

/**
 * @brief Read an 86F header (flux; sector decode is not done in this version).
 */
[[nodiscard]] flux_disk inspect_86f(std::span<const uint8_t> file);

/**
 * @brief Scan a 512-byte boot payload for INT 13h AH=10h / INT 1E hooks.
 */
void add_boot_protection(flux_disk& disk, std::span<const uint8_t> boot);

/**
 * @brief Classify HLS IDs and assemble a 512-byte CHS image from decoded IDAMs.
 *
 * SPT/heads come from a valid boot-DAM BPB (512-byte, SPT 8..36, 1 or 2 heads)
 * when present; otherwise the modal dense 1..N among 512-byte IDs in 1..36.
 * A single extra IDAM does not raise SPT or heads. @a assembled_chs is capped
 * at @c k_max_image_bytes and at BPB @c total_sectors * 512 when known.
 *
 * @param[in,out] disk Sectors and optional @a boot payload already filled.
 */
void finish_ibm_flux(flux_disk& disk);

/**
 * @brief Write changed 512-byte CHS sectors back into an HxC `.mfm` bitstream.
 *
 * Extra HLS IDs (sector outside 1..@a chs_spt or head >= @a chs_heads) are
 * left untouched. @p new_chs must match @a assembled_chs size.
 *
 * @retval true  All changed standard sectors were encoded.
 */
[[nodiscard]] bool patch_mfm_chs(std::vector<uint8_t>& mfm, const flux_disk& flux,
                                 std::span<const uint8_t> new_chs);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_IBM_MFM_HPP */
