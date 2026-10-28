/**
 * @file harness_dos33_chain.c
 * @brief CBMC harness for production @ref dos33_ts_chain.
 *
 * A fully nondeterministic 2-track, 2-sector volume and a nondeterministic
 * start link and cap (<= 3). Proves: no out-of-bounds access, the walk
 * terminates (unwinding assertions), never stores more than cap lists,
 * every stored list is in range and distinct, and OK means the last
 * stored list links to track 0.
 */
#include "dumpfloppy/dos33_chain.h"

#ifdef __CPROVER__
uint8_t nondet_u8(void);
size_t nondet_size(void);
#else
#include <assert.h>
#include <stdlib.h>
#define __CPROVER_assert(cond, msg) assert((cond) && (msg))
#define __CPROVER_assume(cond) do { if (!(cond)) { return 0; } } while (0)
static uint8_t nondet_u8(void) { return (uint8_t)rand(); }
static size_t nondet_size(void) { return (size_t)rand(); }
#endif

#define H_TRACKS 2u
#define H_SPT 2u
#define H_CAP 3u

static uint8_t vol[H_TRACKS * H_SPT * DOS33_CHAIN_SECTOR];

int main(void)
{
    for (size_t i = 0u; i < H_TRACKS * H_SPT; ++i)
    {
        vol[i * DOS33_CHAIN_SECTOR + 1u] = nondet_u8();
        vol[i * DOS33_CHAIN_SECTOR + 2u] = nondet_u8();
    }
    uint8_t out_t[H_CAP];
    uint8_t out_s[H_CAP];
    size_t n = 99u;
    const size_t cap = nondet_size();
    __CPROVER_assume(cap <= H_CAP);
    /* vol_len may also be short: a truncated volume must not be read past. */
    const size_t vol_len = nondet_size();
    __CPROVER_assume(vol_len <= sizeof vol);
    const enum dos33_chain_status st = dos33_ts_chain(
        vol, vol_len, H_TRACKS, H_SPT, nondet_u8(), nondet_u8(), out_t, out_s, cap, &n);

    __CPROVER_assert(n <= cap, "never more than cap lists");
    __CPROVER_assert(st <= DOS33_CHAIN_TOO_LONG, "known status");
    for (size_t i = 0u; i < n; ++i)
    {
        __CPROVER_assert(dos33_ts_in_range(out_t[i], out_s[i], H_TRACKS, H_SPT),
                         "stored list in range");
        for (size_t j = i + 1u; j < n; ++j)
        {
            __CPROVER_assert(out_t[i] != out_t[j] || out_s[i] != out_s[j], "lists distinct");
        }
    }
    if (st == DOS33_CHAIN_OK && n > 0u)
    {
        const size_t last = ((size_t)out_t[n - 1u] * H_SPT + out_s[n - 1u]) * DOS33_CHAIN_SECTOR;
        __CPROVER_assert(vol[last + 1u] == 0u, "OK ends on a track 0 link");
    }
    if (st == DOS33_CHAIN_TOO_LONG)
    {
        __CPROVER_assert(n == cap, "too long only when full");
    }
    __CPROVER_assert(dos33_ts_in_range(0u, 0u, H_TRACKS, H_SPT) == 0, "track 0 never a list");
    return 0;
}
