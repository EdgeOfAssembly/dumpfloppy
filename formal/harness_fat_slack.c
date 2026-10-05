/**
 * @file harness_fat_slack.c
 * @brief CBMC harness for production @ref fat_slack_bytes.
 *
 * Proves the four slack cases: zero size, zero cluster size, an exact
 * multiple, and a non-zero remainder (no wrap).
 */
#include "dumpfloppy/fat_slack.h"

#ifdef __CPROVER__
#else
#include <assert.h>
#define __CPROVER_assert(cond, msg) assert((cond) && (msg))
#endif

int main(void)
{
    __CPROVER_assert(fat_slack_bytes(0u, 512u) == 0u, "zero size");
    __CPROVER_assert(fat_slack_bytes(100u, 0u) == 0u, "zero cluster");
    __CPROVER_assert(fat_slack_bytes(512u, 512u) == 0u, "exact multiple");
    __CPROVER_assert(fat_slack_bytes(500u, 512u) == 12u, "remainder");
    __CPROVER_assert(fat_slack_bytes(1u, 512u) == 511u, "one-byte file");
    /* 0xFFFFFFFF % 0x80000000 == 0x7FFFFFFF; difference is 1, not a wrap. */
    __CPROVER_assert(fat_slack_bytes(0xFFFFFFFFu, 0x80000000u) == 1u, "no wrap");
    return 0;
}
