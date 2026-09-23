/**
 * @file cbm_view.hpp
 * @brief CBMFS result types for @ref analysis (no parser or geometry).
 */
#ifndef DUMPFLOPPY_CBM_VIEW_HPP
#define DUMPFLOPPY_CBM_VIEW_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace dumpfloppy
{

/** @brief Detected CBM disk image kind. */
enum class cbm_media : uint8_t
{
    unknown = 0, /**< Size did not match D64/D71/D81, or parse failed. */
    d64 = 1,     /**< 1541 35-track. */
    d71 = 2,     /**< 1571 70-track. */
    d81 = 3,     /**< 1581 80×40. */
    g64 = 4,     /**< GCR-1541 container decoded to a 35-track D64 map. */
    g71 = 5      /**< GCR-1571 container decoded to a 70-track D71 map. */
};

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

/**
 * @brief Parsed CBMFS disk; @a present is false when size or BAM/header is unusable.
 *
 * @a media / @a media_name are set when @a present is true (`D64` / `D71` /
 * `D81` / `G64` / `G71`).
 */
struct cbm_disk
{
    bool present = false;
    cbm_media media = cbm_media::unknown;
    std::string media_name{}; /**< `D64`, `D71`, `D81`, `G64`, or `G71` when @a present. */
    std::string disk_name{};
    std::string disk_id{};
    uint8_t dos_version = 0; /**< Header byte 2; D64/D71 @c 'A' or 0, D81 @c 'D' or 0. */
    std::string dos_type{};  /**< D64/D71 at 0xA5 (often "2A"); D81 at offset 25 ("3D"). */
    uint8_t dir_track = 0;   /**< Header[0]. */
    uint8_t dir_sector = 0;  /**< Header[1]. */
    std::vector<cbm_file> entries{};
    /**
     * G64: 174848-byte 1541 sector map after GCR decode. G71: 349696-byte
     * 1571 map. Empty for D64/D71/D81 (payloads are read from the raw image).
     */
    std::vector<uint8_t> decoded{};
};

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_CBM_VIEW_HPP */
