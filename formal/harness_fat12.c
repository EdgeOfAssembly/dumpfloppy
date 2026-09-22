/**
 * @file harness_fat12.c
 * @brief CBMC harness: FAT12 even/odd packed entries round-trip.
 */
#include "dumpfloppy/fat12_codec.h"

#ifdef __CPROVER__
uint16_t nondet_uint16_t(void);
#else
#include <assert.h>
#define __CPROVER_assert(cond, msg) assert(cond)
#define __CPROVER_assume(cond)      ((void)0)
#endif

int main(void)
{
    uint8_t fat[3];
    uint16_t a = 0;
    uint16_t b = 0;
    uint16_t ra = 0;
    uint16_t rb = 0;

#ifdef __CPROVER__
    a = nondet_uint16_t();
    b = nondet_uint16_t();
#else
    a = 0x123u;
    b = 0x456u;
#endif
    __CPROVER_assume(a <= FAT12_MASK);
    __CPROVER_assume(b <= FAT12_MASK);

    fat[0] = 0;
    fat[1] = 0;
    fat[2] = 0;

    __CPROVER_assert(fat12_entry_set(fat, sizeof(fat), 0, a) == 0, "set even");
    __CPROVER_assert(fat12_entry_set(fat, sizeof(fat), 1, b) == 0, "set odd");
    __CPROVER_assert(fat12_entry_get(fat, sizeof(fat), 0, &ra) == 0, "get even");
    __CPROVER_assert(fat12_entry_get(fat, sizeof(fat), 1, &rb) == 0, "get odd");
    __CPROVER_assert(ra == a, "even round-trip");
    __CPROVER_assert(rb == b, "odd round-trip");

    uint16_t v = 0;
    __CPROVER_assert(fat12_entry_get(fat, sizeof(fat), 2, &v) == -1, "OOB cluster");
    __CPROVER_assert(fat12_entry_get(NULL, sizeof(fat), 0, &v) == -1, "NULL fat");
    __CPROVER_assert(fat12_entry_get(fat, sizeof(fat), 0, NULL) == -1, "NULL out");

    /* Classic textbook packing. */
    uint8_t classic[3] = {0x23, 0x61, 0x45};
    __CPROVER_assert(fat12_entry_get(classic, 3, 0, &v) == 0, "classic even get");
    __CPROVER_assert(v == 0x123u, "classic even value");
    __CPROVER_assert(fat12_entry_get(classic, 3, 1, &v) == 0, "classic odd get");
    __CPROVER_assert(v == 0x456u, "classic odd value");

    return 0;
}
