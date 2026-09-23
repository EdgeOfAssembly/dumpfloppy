/**
 * @file cbm.hpp
 * @brief CBMFS geometry and parser for 1541 D64, 1571 D71, 1581 D81, G64, G71.
 *
 * 256-byte sectors. D64/D71 use 1541 zone SPT 21/19/18/17 (D71 side 1 is
 * the same table on tracks 36–70). D81 is 80 tracks × 40 sectors. G64 is
 * GCR-1541 decoded onto the 35-track D64 map; G71 is GCR-1571 decoded onto
 * the 70-track D71 map (@ref cbm_disk::decoded). BAM/header: D64/D71/G64/G71
 * at 18/0; D81 at 40/0. @ref analyse stores the result in @c analysis::cbm
 * and skips FAT when @a present is true.
 */
#ifndef DUMPFLOPPY_CBM_HPP
#define DUMPFLOPPY_CBM_HPP

#include "dumpfloppy/cbm_view.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace dumpfloppy
{

/** @brief Data bytes per CBM sector (link T/S occupies the first two). */
inline constexpr uint32_t k_d64_sector_bytes = 256u;

/** @brief Standard 1541 track count (1-based). */
inline constexpr uint8_t k_d64_track_count = 35u;

/** @brief 1571 D71 track count (1-based, both sides). */
inline constexpr uint8_t k_d71_track_count = 70u;

/** @brief 1581 D81 track count (1-based). */
inline constexpr uint8_t k_d81_track_count = 80u;

/** @brief Sectors per track on a 1581 (constant). */
inline constexpr uint8_t k_d81_sectors_per_track = 40u;

/** @brief Directory / BAM track on a 1541 / 1571. */
inline constexpr uint8_t k_d64_bam_track = 18u;

/** @brief Header track on a 1581 (BAM at 40/1–40/2, directory from 40/3). */
inline constexpr uint8_t k_d81_header_track = 40u;

/** @brief 35-track D64 without the optional error map (683 * 256). */
inline constexpr std::size_t k_d64_35_bytes = 174848u;

/** @brief 35-track D64 plus one error byte per sector (174848 + 683). */
inline constexpr std::size_t k_d64_35_error_bytes = 175531u;

/** @brief 70-track D71 without the optional error map (1366 * 256). */
inline constexpr std::size_t k_d71_bytes = 349696u;

/** @brief 70-track D71 plus one error byte per sector (349696 + 1366). */
inline constexpr std::size_t k_d71_error_bytes = 351062u;

/** @brief 80×40 D81 without the optional error map (3200 * 256). */
inline constexpr std::size_t k_d81_bytes = 819200u;

/** @brief 80×40 D81 plus one error byte per sector (819200 + 3200). */
inline constexpr std::size_t k_d81_error_bytes = 822400u;

/** @brief Max file/directory chain visits (one per 1581 sector). */
inline constexpr int k_cbm_max_chain = 3200;

/**
 * @brief Listing label for @p media (`D64` / `D71` / `D81` / `G64` / `G71`; empty if unknown).
 *
 * @param[in] media Image kind.
 */
[[nodiscard]] constexpr const char* cbm_media_name(cbm_media media) noexcept
{
    switch (media)
    {
    case cbm_media::d64:
        return "D64";
    case cbm_media::d71:
        return "D71";
    case cbm_media::d81:
        return "D81";
    case cbm_media::g64:
        return "G64";
    case cbm_media::g71:
        return "G71";
    case cbm_media::unknown:
    default:
        return "";
    }
}

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
 * @brief True for a 70-track D71 size (plain or with error map).
 *
 * @param[in] n Image length in bytes.
 */
[[nodiscard]] constexpr bool is_d71_size(std::size_t n) noexcept
{
    return n == k_d71_bytes || n == k_d71_error_bytes;
}

/**
 * @brief True for an 80×40 D81 size (plain or with error map).
 *
 * @param[in] n Image length in bytes.
 */
[[nodiscard]] constexpr bool is_d81_size(std::size_t n) noexcept
{
    return n == k_d81_bytes || n == k_d81_error_bytes;
}

/**
 * @brief Map a CBM image length to @ref cbm_media.
 *
 * @param[in] n Image length in bytes.
 *
 * @return @ref cbm_media::unknown when @p n is not a D64/D71/D81 size.
 */
[[nodiscard]] constexpr cbm_media cbm_media_from_size(std::size_t n) noexcept
{
    if (is_d64_35_size(n))
    {
        return cbm_media::d64;
    }
    if (is_d71_size(n))
    {
        return cbm_media::d71;
    }
    if (is_d81_size(n))
    {
        return cbm_media::d81;
    }
    return cbm_media::unknown;
}

/**
 * @brief Sectors on @p track for @p media (0 if the track is out of range).
 *
 * D71 tracks 36–70 use the 1541 table for @c track-35.
 *
 * @param[in] media Image kind.
 * @param[in] track 1-based track number.
 */
[[nodiscard]] constexpr uint8_t cbm_sectors_per_track(cbm_media media,
                                                      uint8_t track) noexcept
{
    switch (media)
    {
    case cbm_media::d64:
    case cbm_media::g64:
        return d64_sectors_per_track(track);
    case cbm_media::d71:
    case cbm_media::g71:
        if (track >= 1u && track <= k_d64_track_count)
        {
            return d64_sectors_per_track(track);
        }
        if (track > k_d64_track_count && track <= k_d71_track_count)
        {
            return d64_sectors_per_track(static_cast<uint8_t>(track - k_d64_track_count));
        }
        return 0u;
    case cbm_media::d81:
        if (track >= 1u && track <= k_d81_track_count)
        {
            return k_d81_sectors_per_track;
        }
        return 0u;
    case cbm_media::unknown:
    default:
        return 0u;
    }
}

/**
 * @brief True when @p track/@p sector is legal for @p media.
 *
 * @param[in] media  Image kind.
 * @param[in] track  1-based track.
 * @param[in] sector 0-based sector.
 */
[[nodiscard]] constexpr bool cbm_ts_valid(cbm_media media, uint8_t track,
                                          uint8_t sector) noexcept
{
    const uint8_t spt = cbm_sectors_per_track(media, track);
    return spt != 0u && sector < spt;
}

/**
 * @brief Byte offset of track/sector in a 35-track D64.
 *
 * @param[in] track  1-based (1–35).
 * @param[in] sector 0-based.
 *
 * @return Offset, or @c std::size_t(-1) when the T/S is out of range.
 *
 * @note Track 18 sector 0 is at @c 17*21*256. Track 36 is invalid here;
 *       use @ref cbm_offset with @ref cbm_media::d71.
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
 * @brief Byte offset of track/sector for @p media.
 *
 * @param[in] media  Image kind.
 * @param[in] track  1-based.
 * @param[in] sector 0-based.
 *
 * @return Offset, or @c std::size_t(-1) when the T/S is out of range.
 *
 * @note D71 track 36 sector 0 is at @c 683*256. D81 track 40 sector 0 is
 *       at @c 39*40*256.
 */
[[nodiscard]] constexpr std::size_t cbm_offset(cbm_media media, uint8_t track,
                                               uint8_t sector) noexcept
{
    if (!cbm_ts_valid(media, track, sector))
    {
        return static_cast<std::size_t>(-1);
    }
    if (media == cbm_media::d81)
    {
        const std::size_t sectors =
            (static_cast<std::size_t>(track) - 1u) *
                static_cast<std::size_t>(k_d81_sectors_per_track) +
            static_cast<std::size_t>(sector);
        return sectors * static_cast<std::size_t>(k_d64_sector_bytes);
    }
    if ((media == cbm_media::d71 || media == cbm_media::g71) &&
        track > k_d64_track_count)
    {
        return k_d64_35_bytes +
               d64_offset(static_cast<uint8_t>(track - k_d64_track_count), sector);
    }
    return d64_offset(track, sector);
}

/**
 * @brief Sector image used to walk CBM file chains.
 *
 * G64/G71 payloads live in @a disk.decoded (D64 / D71 layout). D64/D71/D81
 * use @p raw.
 *
 * @param[in] raw  Original image bytes.
 * @param[in] disk Parsed CBM disk (may hold @a decoded).
 */
[[nodiscard]] inline std::span<const uint8_t>
cbm_sector_bytes(std::span<const uint8_t> raw, const cbm_disk& disk) noexcept
{
    if (!disk.decoded.empty())
    {
        return std::span<const uint8_t>(disk.decoded);
    }
    return raw;
}

/**
 * @brief Geometry-aware SPT (1541 wrapper kept as the one-argument overload).
 *
 * @param[in] media Image kind.
 * @param[in] track 1-based track.
 */
[[nodiscard]] constexpr uint8_t d64_sectors_per_track(cbm_media media,
                                                      uint8_t track) noexcept
{
    return cbm_sectors_per_track(media, track);
}

/**
 * @brief Geometry-aware T/S check (1541 wrapper kept as the two-argument overload).
 *
 * @param[in] media  Image kind.
 * @param[in] track  1-based track.
 * @param[in] sector 0-based sector.
 */
[[nodiscard]] constexpr bool d64_ts_valid(cbm_media media, uint8_t track,
                                          uint8_t sector) noexcept
{
    return cbm_ts_valid(media, track, sector);
}

/**
 * @brief Geometry-aware sector offset (1541 wrapper kept as the two-argument overload).
 *
 * @param[in] media  Image kind.
 * @param[in] track  1-based track.
 * @param[in] sector 0-based sector.
 */
[[nodiscard]] constexpr std::size_t d64_offset(cbm_media media, uint8_t track,
                                               uint8_t sector) noexcept
{
    return cbm_offset(media, track, sector);
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
 * @brief Host filename extension for @p kind (`.prg` / `.seq` / `.usr` /
 *        `.rel` / `.del`; `.cbm` for @ref cbm_file_kind::other).
 *
 * @param[in] kind File kind from the directory type nibble.
 */
[[nodiscard]] const char* cbm_file_kind_ext(cbm_file_kind kind) noexcept;

/**
 * @brief PETSCII listing name plus type extension for extract.
 *
 * Path separators in @a file.name become `_`. An empty name is `unnamed`.
 *
 * @param[in] file Directory slot.
 */
[[nodiscard]] std::string cbm_host_filename(const cbm_file& file);

/**
 * @brief Parse CBMFS using @p media geometry.
 *
 * D64/D71: BAM at 18/0, DOS @c 'A' or 0, name at 0x90, ID at 0xA2, type at 0xA5.
 * D81: header at 40/0, DOS @c 'D' or 0, name at 4, ID at 22, type at 25.
 *
 * @param[in] image Whole image (or a span large enough for the header sector).
 * @param[in] media Geometry to apply; @ref cbm_media::unknown yields not present.
 *
 * @return @a present false if the header sector is missing or DOS/dir T/S is invalid.
 */
[[nodiscard]] cbm_disk parse_cbmfs(std::span<const uint8_t> image, cbm_media media);

/**
 * @brief Parse CBMFS, inferring geometry from image size (D64 fallback).
 *
 * Known D64/D71/D81 sizes use that media; any other length tries 1541 BAM
 * at 18/0 (legacy @ref parse_cbmfs behaviour).
 *
 * @param[in] image Whole image.
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
 * @brief Parse a 70-track D71 and its CBMFS (BAM still at 18/0).
 *
 * @param[in] image Image bytes; size must be 349696 or 351062.
 *
 * @return @a present false when the size is not a D71 or BAM is invalid.
 */
[[nodiscard]] cbm_disk parse_d71(std::span<const uint8_t> image);

/**
 * @brief Parse an 80×40 D81 and its CBMFS (header at 40/0, BAM at 40/1).
 *
 * Header DOS @c 'D' or 0, byte 3 @c $00, and BAM 40/1 DOS plus ones-complement
 * ($44/$BB). Size 819200 also matches IBM 800K; the BAM pair keeps FAT disks.
 *
 * @param[in] image Image bytes; size must be 819200 or 822400.
 *
 * @return @a present false when the size is not a D81 or the header/BAM is invalid.
 */
[[nodiscard]] cbm_disk parse_d81(std::span<const uint8_t> image);

/**
 * @brief Pick D64/D71/D81 by image size and parse CBMFS.
 *
 * @param[in] image Whole image.
 *
 * @return @a present false when the size is not a known CBM image or the
 *         BAM/header is invalid.
 */
[[nodiscard]] cbm_disk parse_cbm_image(std::span<const uint8_t> image);

/**
 * @brief Follow a CBM file chain (254 data bytes/sector; last T=0, S=last used byte).
 *
 * Geometry is inferred from @p image size (D64 fallback).
 *
 * @param[in] image Whole image.
 * @param[in] track First data track.
 * @param[in] sector First data sector.
 *
 * @return Payload bytes (empty when the start T/S is 0 or unreadable).
 *
 * @note Stops on cycles, invalid T/S, or a truncated sector. Deleted files
 *       still follow their stored chain. Chain cap is @ref k_cbm_max_chain.
 */
[[nodiscard]] std::vector<uint8_t>
read_cbm_file(std::span<const uint8_t> image, uint8_t track, uint8_t sector);

/**
 * @brief Follow a CBM file chain with explicit @p media geometry.
 *
 * @param[in] image Whole image.
 * @param[in] media Geometry (D71 accepts tracks 36–70; D81 is 1–80, SPT 40).
 * @param[in] track First data track.
 * @param[in] sector First data sector.
 */
[[nodiscard]] std::vector<uint8_t> read_cbm_file(std::span<const uint8_t> image,
                                                 cbm_media media, uint8_t track,
                                                 uint8_t sector);

/**
 * @brief Read the payload for a directory entry (geometry from image size).
 *
 * @param[in] image Whole image.
 * @param[in] file  Directory slot (uses @a first_track / @a first_sector).
 */
[[nodiscard]] std::vector<uint8_t> read_cbm_file(std::span<const uint8_t> image,
                                                 const cbm_file& file);

/**
 * @brief Read the payload for a directory entry with explicit @p media.
 *
 * @param[in] image Whole image.
 * @param[in] media Geometry.
 * @param[in] file  Directory slot.
 */
[[nodiscard]] std::vector<uint8_t> read_cbm_file(std::span<const uint8_t> image,
                                                 cbm_media media,
                                                 const cbm_file& file);

/**
 * @brief Overwrite a CBM file chain in place when @p payload matches on-disk size.
 *
 * Leaves T/S links and the last-sector used-byte index unchanged. REL files
 * and G64 (no sector image in @p image) are rejected by the caller.
 *
 * @param[in,out] image   D64/D71/D81 bytes.
 * @param[in]     media   Geometry.
 * @param[in]     file    Live directory slot.
 * @param[in]     payload Host bytes; length must equal @ref read_cbm_file.
 * @param[out]    err     Reason on failure.
 *
 * @retval true  Payload written.
 * @retval false Size mismatch, bad T/S, or truncated sector; @p err set.
 */
[[nodiscard]] bool write_cbm_file_same_size(std::vector<uint8_t>& image,
                                            cbm_media media, const cbm_file& file,
                                            std::span<const uint8_t> payload,
                                            std::string& err);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_CBM_HPP */
