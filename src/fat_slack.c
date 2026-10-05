/**
 * @file fat_slack.c
 * @brief Slack length for one FAT file. Production callers must use this.
 */
#include "dumpfloppy/fat_slack.h"

uint32_t fat_slack_bytes(uint32_t file_size, uint32_t cluster_bytes)
{
    uint32_t rem = 0;

    if (cluster_bytes == 0u || file_size == 0u)
    {
        return 0u;
    }
    rem = file_size % cluster_bytes;
    if (rem == 0u)
    {
        return 0u;
    }
    return cluster_bytes - rem;
}
