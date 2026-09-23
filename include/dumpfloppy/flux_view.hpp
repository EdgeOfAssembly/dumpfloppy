/**
 * @file flux_view.hpp
 * @brief HxC/86F result types for @ref analysis (no bitstream decoder).
 */
#ifndef DUMPFLOPPY_FLUX_VIEW_HPP
#define DUMPFLOPPY_FLUX_VIEW_HPP

#include <cstddef>
#include <cstdint>
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
    std::vector<uint8_t> data{}; /**< DAM payload when present. */
    std::size_t track_file_off = 0; /**< Byte offset of this track in the .mfm. */
    std::size_t track_byte_len = 0;
    std::size_t dam_bit_off = 0;    /**< Bit index of DAM payload in the track. */
    uint8_t dam_mark = 0xFB;
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
    std::vector<uint8_t> assembled_chs{}; /**< Standard 512-byte CHS image, if any. */
    uint32_t chs_cyls = 0;
    uint32_t chs_heads = 0;
    uint32_t chs_spt = 0; /**< BPB or modal SPT (8..36); extra HLS IDs are omitted. */
};

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_FLUX_VIEW_HPP */
