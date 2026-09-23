/**
 * @file harness_gcr.c
 * @brief CBMC harness: Commodore GCR 4-to-5 nibble and byte round-trip.
 */
#include "dumpfloppy/gcr_codec.h"

#include <stddef.h>

#ifdef __CPROVER__
uint8_t nondet_uint8_t(void);
#else
#include <assert.h>
#define __CPROVER_assert(cond, msg) assert(cond)
#define __CPROVER_assume(cond)      ((void)0)
#endif

int main(void)
{
    uint8_t n = 0;
    uint8_t five = 0;
    uint8_t b = 0;
    uint8_t hi = 0;
    uint8_t lo = 0;
    uint8_t out = 0;

#ifdef __CPROVER__
    n = nondet_uint8_t();
    five = nondet_uint8_t();
    b = nondet_uint8_t();
#endif
    __CPROVER_assume(n <= GCR_NIBBLE_MASK);
    __CPROVER_assume(five <= GCR_FIVE_MASK);

    __CPROVER_assert(gcr_decode_nibble(gcr_encode_nibble(n)) == n,
                     "nibble round-trip");
    __CPROVER_assert((gcr_encode_nibble(n) & (uint8_t)~GCR_FIVE_MASK) == 0u,
                     "encode is 5-bit");

    {
        const uint8_t d = gcr_decode_nibble(five);
        if (d != GCR_INVALID)
        {
            __CPROVER_assert(d <= GCR_NIBBLE_MASK, "decoded nibble range");
            __CPROVER_assert(gcr_encode_nibble(d) == five, "nibble inverse");
        }
    }

    gcr_encode_byte(b, &hi, &lo);
    __CPROVER_assert(gcr_decode_byte(hi, lo, &out) == 0, "byte decode");
    __CPROVER_assert(out == b, "byte round-trip");
    __CPROVER_assert(gcr_decode_byte(hi, lo, NULL) == -1, "NULL out");

    /* Textbook codes: 0→01010, F→10101, 8→01001 (Schepers G64/ZIP_SIX). */
    __CPROVER_assert(gcr_encode_nibble(0x0u) == 0x0Au, "encode 0");
    __CPROVER_assert(gcr_encode_nibble(0x8u) == 0x09u, "encode 8");
    __CPROVER_assert(gcr_encode_nibble(0xFu) == 0x15u, "encode F");
    __CPROVER_assert(gcr_decode_nibble(0x00u) == GCR_INVALID, "invalid 0");
    __CPROVER_assert(gcr_decode_nibble(0x1Cu) == GCR_INVALID, "invalid 1C");
    __CPROVER_assert(gcr_decode_nibble(0x1Fu) == GCR_INVALID, "invalid 1F sync");

    return 0;
}
