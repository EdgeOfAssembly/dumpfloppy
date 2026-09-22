/**
 * @file fat12_codec.h
 * @brief Bounded FAT12 12-bit entry get/set (C23, CBMC-friendly).
 *
 * Cluster index @p cluster occupies 12 bits packed little-endian:
 * byte offset = cluster + cluster/2. Even clusters use the low nibble
 * of the 16-bit word; odd clusters use the high nibble.
 */
#ifndef DUMPFLOPPY_FAT12_CODEC_H
#define DUMPFLOPPY_FAT12_CODEC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief FAT12 free cluster marker. */
#define FAT12_FREE 0x000u
/** @brief FAT12 bad-cluster marker. */
#define FAT12_BAD 0xFF7u
/** @brief First FAT12 reserved marker (0xFF0–0xFF6). */
#define FAT12_RESERVED_MIN 0xFF0u
/** @brief Last FAT12 reserved marker. */
#define FAT12_RESERVED_MAX 0xFF6u
/** @brief First FAT12 end-of-chain marker (0xFF8–0xFFF). */
#define FAT12_EOF_MIN 0xFF8u
/** @brief 12-bit mask. */
#define FAT12_MASK 0x0FFFu

/**
 * @brief Read one FAT12 entry.
 *
 * @param[in]  fat      FAT bytes (must not be NULL).
 * @param[in]  fat_len  Size of @p fat in bytes.
 * @param[in]  cluster  Cluster index (0-based FAT slot).
 * @param[out] out      Decoded 12-bit value; only written on success.
 *
 * @retval  0 Success.
 * @retval -1 NULL pointer, out-of-range cluster, or truncated FAT.
 */
int fat12_entry_get(const uint8_t *fat, size_t fat_len, uint32_t cluster,
                    uint16_t *out);

/**
 * @brief Write one FAT12 entry without disturbing the packed neighbour.
 *
 * @param[in,out] fat      FAT bytes (must not be NULL).
 * @param[in]     fat_len  Size of @p fat in bytes.
 * @param[in]     cluster  Cluster index (0-based FAT slot).
 * @param[in]     value    Stored as @p value & 0x0FFF.
 *
 * @retval  0 Success.
 * @retval -1 NULL pointer, out-of-range cluster, or truncated FAT.
 */
int fat12_entry_set(uint8_t *fat, size_t fat_len, uint32_t cluster,
                    uint16_t value);

/**
 * @brief Byte offset of the 16-bit window that contains @p cluster.
 *
 * @param[in] cluster Cluster index.
 * @return `cluster + cluster/2`.
 */
static inline size_t fat12_entry_offset(uint32_t cluster)
{
    return (size_t)cluster + (size_t)(cluster / 2u);
}

static inline int fat12_is_free(uint16_t value)
{
    return value == FAT12_FREE;
}

static inline int fat12_is_bad(uint16_t value)
{
    return value == FAT12_BAD;
}

static inline int fat12_is_eof(uint16_t value)
{
    return value >= FAT12_EOF_MIN;
}

static inline int fat12_is_reserved(uint16_t value)
{
    return value >= FAT12_RESERVED_MIN && value <= FAT12_RESERVED_MAX;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DUMPFLOPPY_FAT12_CODEC_H */
