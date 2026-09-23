/**
 * @file gcr_codec.h
 * @brief Commodore 1541 4-to-5 GCR nibble codec (C23, CBMC-friendly).
 *
 * Each data nibble maps to a 5-bit group so the on-disk stream never uses
 * the long run of 1-bits reserved for SYNC (nibble F is 10101, not 11111).
 * Invalid 5-bit codes decode to @ref GCR_INVALID.
 */
#ifndef DUMPFLOPPY_GCR_CODEC_H
#define DUMPFLOPPY_GCR_CODEC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Sentinel from @ref gcr_decode_nibble when the 5-bit group is unused. */
#define GCR_INVALID 0xFFu

/** @brief 4-bit data mask. */
#define GCR_NIBBLE_MASK 0x0Fu

/** @brief 5-bit GCR group mask. */
#define GCR_FIVE_MASK 0x1Fu

/**
 * @brief Encode one data nibble to a 5-bit GCR group.
 *
 * @param[in] nibble Low 4 bits are used; high bits are ignored.
 *
 * @return Value in 0x00–0x1F.
 */
uint8_t gcr_encode_nibble(uint8_t nibble);

/**
 * @brief Decode one 5-bit GCR group to a data nibble.
 *
 * @param[in] five Low 5 bits are used; high bits are ignored.
 *
 * @return Nibble 0–15, or @ref GCR_INVALID when @p five is not a GCR code.
 */
uint8_t gcr_decode_nibble(uint8_t five);

/**
 * @brief Encode one data byte to two 5-bit GCR groups (high nibble first).
 *
 * @param[in]  value Data byte.
 * @param[out] hi5   High-nibble GCR group; ignored when NULL.
 * @param[out] lo5   Low-nibble GCR group; ignored when NULL.
 */
void gcr_encode_byte(uint8_t value, uint8_t *hi5, uint8_t *lo5);

/**
 * @brief Decode two 5-bit GCR groups to one data byte.
 *
 * @param[in]  hi5 High-nibble GCR group.
 * @param[in]  lo5 Low-nibble GCR group.
 * @param[out] out Decoded byte; only written on success.
 *
 * @retval  0 Success.
 * @retval -1 @p out is NULL, or either group is @ref GCR_INVALID.
 */
int gcr_decode_byte(uint8_t hi5, uint8_t lo5, uint8_t *out);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DUMPFLOPPY_GCR_CODEC_H */
