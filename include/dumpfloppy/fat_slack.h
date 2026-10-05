/**
 * @file fat_slack.h
 * @brief Unused tail of a FAT file's last cluster (C23).
 */
#ifndef DUMPFLOPPY_FAT_SLACK_H
#define DUMPFLOPPY_FAT_SLACK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bytes of slack after a file in its last cluster.
 *
 * @param file_size     Logical file size in bytes.
 * @param cluster_bytes Bytes per cluster (0 is rejected).
 *
 * @return 0 when @p cluster_bytes is 0, @p file_size is 0, or @p file_size
 *         is an exact multiple of @p cluster_bytes. Otherwise
 *         @p cluster_bytes minus (@p file_size modulo @p cluster_bytes).
 *         The subtraction does not wrap.
 */
uint32_t fat_slack_bytes(uint32_t file_size, uint32_t cluster_bytes);

#ifdef __cplusplus
}
#endif

#endif /* DUMPFLOPPY_FAT_SLACK_H */
