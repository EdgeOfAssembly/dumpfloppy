/**
 * @file harness_apple_gcr.c
 * @brief CBMC harness: Apple 6-and-2 nibble map and 4-and-4 round-trip.
 */
#include "dumpfloppy/apple_gcr_codec.h"

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
    uint8_t six = 0;
    uint8_t disk = 0;
    uint8_t value = 0;
    uint8_t odd = 0;
    uint8_t even = 0;
    uint8_t out = 0;

#ifdef __CPROVER__
    six = nondet_uint8_t();
    disk = nondet_uint8_t();
    value = nondet_uint8_t();
#endif
    __CPROVER_assume(six <= APPLE_GCR_SIX_MASK);

    __CPROVER_assert(apple_gcr_decode_6n2(apple_gcr_encode_6n2(six)) == six,
                     "6-and-2 nibble round-trip");
    __CPROVER_assert((apple_gcr_encode_6n2(six) & 0x80u) != 0u,
                     "encoded GCR has high bit");

    {
        const uint8_t d = apple_gcr_decode_6n2(disk);
        if (d != APPLE_GCR_INVALID)
        {
            __CPROVER_assert(d <= APPLE_GCR_SIX_MASK, "decoded 6-bit range");
            __CPROVER_assert(apple_gcr_encode_6n2(d) == disk, "6-and-2 inverse");
        }
    }

    apple_gcr_encode_4n4(value, &odd, &even);
    __CPROVER_assert(apple_gcr_decode_4n4(odd, even, &out) == 0, "4-and-4 decode");
    __CPROVER_assert(out == value, "4-and-4 round-trip");
    __CPROVER_assert(apple_gcr_decode_4n4(odd, even, NULL) == -1, "NULL out");
    __CPROVER_assert((odd & 0xAAu) == 0xAAu, "odd has AA bits");
    __CPROVER_assert((even & 0xAAu) == 0xAAu, "even has AA bits");

    /* Textbook 6-and-2: 0→0x96, 0x3F→0xFF; 4-and-4 of 0 is AA AA. */
    __CPROVER_assert(apple_gcr_encode_6n2(0x00u) == 0x96u, "encode 0");
    __CPROVER_assert(apple_gcr_encode_6n2(0x3Fu) == 0xFFu, "encode 3F");
    __CPROVER_assert(apple_gcr_decode_6n2(0x00u) == APPLE_GCR_INVALID, "invalid 0");
    __CPROVER_assert(apple_gcr_decode_6n2(0xD5u) == APPLE_GCR_INVALID,
                     "prologue D5 unused");

    return 0;
}
