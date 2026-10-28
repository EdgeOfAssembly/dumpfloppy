/**
 * @file dos33_chain.h
 * @brief Bounded, checked walk of a DOS 3.3 track/sector-list chain (C23).
 *
 * Used by `--undelete` before any T/S list is read. Each list sector
 * links to the next one through bytes $01 (track) and $02 (sector); track
 * 0 ends the chain. CBMC proves the walk in formal/harness_dos33_chain.c.
 */
#ifndef DUMPFLOPPY_DOS33_CHAIN_H
#define DUMPFLOPPY_DOS33_CHAIN_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief DOS 3.3 sector size. */
#define DOS33_CHAIN_SECTOR 256u

/**
 * @brief Most T/S lists `--undelete` follows for one file.
 *
 * One list maps 122 sectors, so 6 lists already cover a 40-track
 * 16-sector volume. 64 leaves room for sparse random-access files.
 */
#define DOS33_MAX_TS_LISTS 64u

/** @brief How a T/S list chain walk ended. */
enum dos33_chain_status
{
    DOS33_CHAIN_OK = 0,       /**< Reached a track 0 link. */
    DOS33_CHAIN_BAD_LINK = 1, /**< A link's track or sector is outside the volume. */
    DOS33_CHAIN_LOOP = 2,     /**< A list sector was reached twice. */
    DOS33_CHAIN_TOO_LONG = 3  /**< More than @p cap lists. */
};

/**
 * @brief True when T @p t / S @p s can hold a T/S list (track 0 never can).
 */
int dos33_ts_in_range(uint8_t t, uint8_t s, uint8_t tracks, uint8_t spt);

/**
 * @brief Collect the list sectors of the chain that starts at @p t / @p s.
 *
 * Every link is range-checked against @p tracks, @p spt and @p vol_len
 * before its sector is read. The walk stops at the first bad link, at
 * the first repeated sector, or when @p cap lists are stored and the
 * chain continues. It always returns after at most @p cap + 1 steps.
 *
 * @param[in]  vol     DOS-order volume, 256-byte sectors.
 * @param[in]  vol_len Bytes in @p vol.
 * @param[in]  tracks  Tracks on the volume (VTOC $34).
 * @param[in]  spt     Sectors per track (VTOC $35).
 * @param[in]  t       First list track (0 = empty chain).
 * @param[in]  s       First list sector.
 * @param[out] out_t   Track of list k, for k < *@p n.
 * @param[out] out_s   Sector of list k, for k < *@p n.
 * @param[in]  cap     Room in @p out_t and @p out_s.
 * @param[out] n       Lists stored (always <= @p cap).
 *
 * @return How the walk ended. The lists stored before a bad link, loop
 *         or cap are valid and distinct.
 */
enum dos33_chain_status dos33_ts_chain(const uint8_t* vol, size_t vol_len, uint8_t tracks,
                                       uint8_t spt, uint8_t t, uint8_t s, uint8_t* out_t,
                                       uint8_t* out_s, size_t cap, size_t* n);

#ifdef __cplusplus
}
#endif

#endif /* DUMPFLOPPY_DOS33_CHAIN_H */
