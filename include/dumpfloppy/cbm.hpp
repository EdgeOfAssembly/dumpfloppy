/**
 * @file cbm.hpp
 * @brief 1541 D64 geometry and CBMFS (BAM / directory / file chains).
 *
 * 35-track Commodore 1541 images: 256-byte sectors, zone SPT 21/19/18/17.
 * BAM lives at track 18 sector 0. Not wired into @c analyse this wave.
 */
#ifndef DUMPFLOPPY_CBM_HPP
#define DUMPFLOPPY_CBM_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace dumpfloppy
{

/** @brief Data bytes per 1541 sector (link T/S occupies the first two). */
inline constexpr uint32_t k_d64_sector_bytes = 256u;

/** @brief Standard 1541 track count (1-based). */
inline constexpr uint8_t k_d64_track_count = 35u;

/** @brief Directory / BAM track on a 1541. */
inline constexpr uint8_t k_d64_bam_track = 18u;

/** @brief 35-track D64 without the optional error map (683 * 256). */
inline constexpr std::size_t k_d64_35_bytes = 174848u;

/** @brief 35-track D64 plus one error byte per sector (174848 + 683). */
inline constexpr std::size_t k_d64_35_error_bytes = 175531u;

/** @brief CBM DOS file type in bits 0–3 of the directory type byte. */
enum class cbm_file_kind : uint8_t
{
    del = 0, /**< Deleted / DEL. */
    seq = 1, /**< Sequential. */
    prg = 2, /**< Program. */
    usr = 3, /**< User. */
    rel = 4, /**< Relative. */
    other = 5 /**< Bits 0–3 outside 0–4. */
};

/**
 * @brief One CBMFS directory slot (live or deleted).
 *
 * @a deleted is set when bit 7 of @a type_byte is clear (splat or scratched).
 */
struct cbm_file
{
    std::string name{}; /**< PETSCII converted for listing; 0xA0 pad stripped. */
    uint8_t type_byte = 0;
    cbm_file_kind kind = cbm_file_kind::del;
    bool closed = false;  /**< Type bit 7. */
    bool locked = false;  /**< Type bit 6. */
    bool deleted = false; /**< Bit 7 clear. */
    uint8_t first_track = 0;
    uint8_t first_sector = 0;
    uint16_t size_sectors = 0; /**< Directory bytes 30–31, little-endian. */
};

/** @brief Parsed 1541 CBMFS disk; @a present is false when size or BAM is unusable. */
struct cbm_disk
{
    bool present = false;
    std::string disk_name{};
    std::string disk_id{};
    uint8_t dos_version = 0; /**< BAM byte 2; typically @c 'A' or 0. */
    std::string dos_type{};  /**< BAM 0xA5–0xA6 (often "2A"). */
    uint8_t dir_track = 0;   /**< BAM[0]. */
    uint8_t dir_sector = 0;  /**< BAM[1]. */
    std::vector<cbm_file> entries{};
};

/**
 * @brief Sectors on a 1541 track (0 if @p track is outside 1–35).
 *
 * Zones: 1–17 → 21, 18–24 → 19, 25–30 → 18, 31–35 → 17.
 *
 * @param[in] track 1-based track number.
 */
[[nodiscard]] constexpr uint8_t d64_sectors_per_track(uint8_t track) noexcept
{
    if (track < 1u || track > k_d64_track_count)
    {
        return 0u;
    }
    if (track <= 17u)
    {
        return 21u;
    }
    if (track <= 24u)
    {
        return 19u;
    }
    if (track <= 30u)
    {
        return 18u;
    }
    return 17u;
}

/**
 * @brief True when @p track/@p sector is a legal 1541 address.
 *
 * @param[in] track  1-based track.
 * @param[in] sector 0-based sector.
 */
[[nodiscard]] constexpr bool d64_ts_valid(uint8_t track, uint8_t sector) noexcept
{
    const uint8_t spt = d64_sectors_per_track(track);
    return spt != 0u && sector < spt;
}

/**
 * @brief True for a 35-track D64 size (plain or with error map).
 *
 * @param[in] n Image length in bytes.
 */
[[nodiscard]] constexpr bool is_d64_35_size(std::size_t n) noexcept
{
    return n == k_d64_35_bytes || n == k_d64_35_error_bytes;
}

/**
 * @brief Byte offset of track/sector in a 35-track D64.
 *
 * @param[in] track  1-based (1–35).
 * @param[in] sector 0-based.
 *
 * @return Offset, or @c std::size_t(-1) when the T/S is out of range.
 *
 * @note Track 18 sector 0 is at @c 17*21*256.
 */
[[nodiscard]] constexpr std::size_t d64_offset(uint8_t track, uint8_t sector) noexcept
{
    if (!d64_ts_valid(track, sector))
    {
        return static_cast<std::size_t>(-1);
    }
    std::size_t sectors = 0;
    for (uint8_t t = 1u; t < track; ++t)
    {
        sectors += static_cast<std::size_t>(d64_sectors_per_track(t));
    }
    sectors += static_cast<std::size_t>(sector);
    return sectors * static_cast<std::size_t>(k_d64_sector_bytes);
}

/**
 * @brief PETSCII filename / disk label to listing ASCII.
 *
 * Keeps A–Z, 0–9, dash, and other $20–$5F overlap; maps shifted $C1–$DA
 * to A–Z; stops at $A0 padding or NUL; strips trailing spaces.
 *
 * @param[in] petscii On-disk bytes (typically 16).
 */
[[nodiscard]] std::string petscii_to_ascii(std::span<const uint8_t> petscii);

/**
 * @brief Three-letter type label (`PRG`, `SEQ`, …).
 *
 * @param[in] kind File kind from the directory type nibble.
 */
[[nodiscard]] const char* cbm_file_kind_name(cbm_file_kind kind) noexcept;

/**
 * @brief Parse CBMFS using 1541 T/S geometry (BAM at 18/0).
 *
 * @param[in] image Whole D64 (or a span large enough to hold 18/0).
 *
 * @return @a present false if the BAM sector is missing or not 1541 DOS
 *         (`A` / 0) with a valid directory T/S in BAM[0..1].
 */
[[nodiscard]] cbm_disk parse_cbmfs(std::span<const uint8_t> image);

/**
 * @brief Parse a 35-track D64 and its CBMFS.
 *
 * @param[in] image Image bytes; size must be 174848 or 175531.
 *
 * @return @a present false when the size is not a 35-track D64 or BAM is invalid.
 */
[[nodiscard]] cbm_disk parse_d64(std::span<const uint8_t> image);

/**
 * @brief Follow a CBM file chain (254 data bytes/sector; last T=0, S=last used byte).
 *
 * @param[in] image Whole image.
 * @param[in] track First data track.
 * @param[in] sector First data sector.
 *
 * @return Payload bytes (empty when the start T/S is 0 or unreadable).
 *
 * @note Stops on cycles, invalid T/S, or a truncated sector. Deleted files
 *       still follow their stored chain.
 */
[[nodiscard]] std::vector<uint8_t>
read_cbm_file(std::span<const uint8_t> image, uint8_t track, uint8_t sector);

/**
 * @brief Read the payload for a directory entry.
 *
 * @param[in] image Whole image.
 * @param[in] file  Directory slot (uses @a first_track / @a first_sector).
 */
[[nodiscard]] std::vector<uint8_t> read_cbm_file(std::span<const uint8_t> image,
                                                 const cbm_file& file);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_CBM_HPP */
