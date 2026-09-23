/**
 * @file apple_gcr_codec.h
 * @brief Apple II 6-and-2 GCR and 4-and-4 address codec (C23, CBMC-friendly).
 *
 * Disk II 16-sector encoding (DOS 3.3 / ProDOS 5.25): each on-disk byte
 * holds six data bits via @ref apple_gcr_encode_6n2. Address fields use
 * 4-and-4 (odd/even) so every disk byte has its high bit set.
 *
 * Sector layout after `D5 AA AD` is 342 XOR-chained 6-bit values plus a
 * checksum nibble (343 bytes), matching Clock Signal `six_and_two_data`.
 */
#ifndef DUMPFLOPPY_APPLE_GCR_CODEC_H
#define DUMPFLOPPY_APPLE_GCR_CODEC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Sentinel from @ref apple_gcr_decode_6n2 when the disk byte is unused. */
#define APPLE_GCR_INVALID 0xFFu

/** @brief Six data bits packed into one GCR group. */
#define APPLE_GCR_SIX_MASK 0x3Fu

/** @brief Decoded DOS 3.3 / ProDOS 5.25 sector size. */
#define APPLE_GCR_SECTOR_BYTES 256u

/** @brief Encoded 6-and-2 payload after the data prologue (342 + checksum). */
#define APPLE_GCR_NIBBLE_BYTES 343u

/**
 * @brief Encode six data bits to one Disk II GCR byte.
 *
 * @param[in] six Low 6 bits are used; high bits are ignored.
 *
 * @return On-disk byte (always has bit 7 set).
 */
uint8_t apple_gcr_encode_6n2(uint8_t six);

/**
 * @brief Decode one Disk II GCR byte to six data bits.
 *
 * @param[in] disk On-disk byte.
 *
 * @return Value 0–63, or @ref APPLE_GCR_INVALID when @p disk is not a 6-and-2 code.
 */
uint8_t apple_gcr_decode_6n2(uint8_t disk);

/**
 * @brief Encode one byte as a 4-and-4 odd/even pair (address field).
 *
 * @param[in]  value Data byte.
 * @param[out] odd   `(value >> 1) | 0xAA`; ignored when NULL.
 * @param[out] even  `value | 0xAA`; ignored when NULL.
 */
void apple_gcr_encode_4n4(uint8_t value, uint8_t *odd, uint8_t *even);

/**
 * @brief Decode a 4-and-4 odd/even pair.
 *
 * @param[in]  odd  First on-disk byte (`(value >> 1) | 0xAA`).
 * @param[in]  even Second on-disk byte (`value | 0xAA`).
 * @param[out] out  Decoded byte; only written on success.
 *
 * @retval  0 Success.
 * @retval -1 @p out is NULL.
 */
int apple_gcr_decode_4n4(uint8_t odd, uint8_t even, uint8_t *out);

/**
 * @brief Encode 256 sector bytes to 343 6-and-2 disk bytes (no prologue).
 *
 * @param[in]  src 256-byte sector; must not be NULL.
 * @param[out] dst 343-byte GCR payload; must not be NULL.
 *
 * @retval  0 Success.
 * @retval -1 @p src or @p dst is NULL.
 */
int apple_gcr_encode_sector(const uint8_t *src, uint8_t *dst);

/**
 * @brief Decode 343 6-and-2 disk bytes to a 256-byte sector.
 *
 * @param[in]  src 343-byte GCR payload; must not be NULL.
 * @param[out] dst 256-byte sector; only fully written on success.
 *
 * @retval  0 Success (checksum matched).
 * @retval -1 NULL pointer, invalid GCR byte, or checksum mismatch.
 */
int apple_gcr_decode_sector(const uint8_t *src, uint8_t *dst);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DUMPFLOPPY_APPLE_GCR_CODEC_H */
