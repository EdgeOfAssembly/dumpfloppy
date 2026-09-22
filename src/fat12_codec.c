/**
 * @file fat12_codec.c
 * @brief FAT12 packed-entry codec used by dumpfloppy and CBMC harnesses.
 */
#include "dumpfloppy/fat12_codec.h"

int fat12_entry_get(const uint8_t *fat, size_t fat_len, uint32_t cluster,
                    uint16_t *out)
{
    const size_t off = fat12_entry_offset(cluster);
    uint16_t raw = 0;

    if (fat == NULL || out == NULL)
    {
        return -1;
    }
    if (off + 1u >= fat_len)
    {
        return -1;
    }

    raw = (uint16_t)fat[off] | (uint16_t)((uint16_t)fat[off + 1u] << 8);
    if ((cluster & 1u) != 0u)
    {
        *out = (uint16_t)(raw >> 4);
    }
    else
    {
        *out = (uint16_t)(raw & FAT12_MASK);
    }
    return 0;
}

int fat12_entry_set(uint8_t *fat, size_t fat_len, uint32_t cluster,
                    uint16_t value)
{
    const size_t off = fat12_entry_offset(cluster);
    uint16_t raw = 0;
    const uint16_t clipped = (uint16_t)(value & FAT12_MASK);

    if (fat == NULL)
    {
        return -1;
    }
    if (off + 1u >= fat_len)
    {
        return -1;
    }

    raw = (uint16_t)fat[off] | (uint16_t)((uint16_t)fat[off + 1u] << 8);
    if ((cluster & 1u) != 0u)
    {
        raw = (uint16_t)((raw & 0x000Fu) | (uint16_t)(clipped << 4));
    }
    else
    {
        raw = (uint16_t)((raw & 0xF000u) | clipped);
    }
    fat[off] = (uint8_t)(raw & 0xFFu);
    fat[off + 1u] = (uint8_t)((raw >> 8) & 0xFFu);
    return 0;
}
