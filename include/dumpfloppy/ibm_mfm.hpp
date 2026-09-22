/**
 * @file ibm_mfm.hpp
 * @brief IBM MFM sector map and copy-protection heuristics (HxC .mfm).
 */
#ifndef DUMPFLOPPY_IBM_MFM_HPP
#define DUMPFLOPPY_IBM_MFM_HPP

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace dumpfloppy
{

/** @brief One IBM IDAM (+ DAM CRC if a data field follows). */
struct ibm_sector
{
    uint8_t cyl = 0;
    uint8_t head = 0;
    uint8_t sector = 0;
    uint8_t size_code = 0; /**< 0=128 … 2=512, 3=1024. */
    uint16_t bytes = 0;
    bool idam_crc_ok = false;
    bool dam_crc_ok = false;
    bool has_dam = false;
};

/** @brief Decoded HxC MFM (or empty if the bytes are not that container). */
struct flux_disk
{
    bool present = false;
    std::string format_name{}; /**< `HXC MFM` or `86BOX 86F`. */
    uint32_t tracks = 0;
    uint32_t sides = 0;
    uint32_t rpm = 0;
    uint32_t bitrate_kbps = 0;
    std::vector<ibm_sector> sectors{};
    std::vector<uint8_t> boot{}; /**< Cyl 0 head 0 sector 1 payload, if any. */
    std::vector<std::string> protection{};
    std::string note{};
};

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

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_IBM_MFM_HPP */
