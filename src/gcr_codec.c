/**
 * @file gcr_codec.c
 * @brief Commodore 1541 4-to-5 GCR tables used by dumpfloppy and CBMC.
 *
 * Codes match Schepers `G64.TXT` / `ZIP_SIX.TXT` (nibble F is 10101, not
 * 11111, so two F nibbles cannot form a 10-bit SYNC run).
 */
#include "dumpfloppy/gcr_codec.h"

#include <stddef.h>

static const uint8_t k_gcr_encode[16] = {
    0x0Au, 0x0Bu, 0x12u, 0x13u, 0x0Eu, 0x0Fu, 0x16u, 0x17u,
    0x09u, 0x19u, 0x1Au, 0x1Bu, 0x0Du, 0x1Du, 0x1Eu, 0x15u};

static const uint8_t k_gcr_decode[32] = {
    GCR_INVALID, GCR_INVALID, GCR_INVALID, GCR_INVALID, GCR_INVALID,
    GCR_INVALID, GCR_INVALID, GCR_INVALID, GCR_INVALID, 0x08u,
    0x00u,       0x01u,       GCR_INVALID, 0x0Cu,       0x04u,
    0x05u,       GCR_INVALID, GCR_INVALID, 0x02u,       0x03u,
    GCR_INVALID, 0x0Fu,       0x06u,       0x07u,       GCR_INVALID,
    0x09u,       0x0Au,       0x0Bu,       GCR_INVALID, 0x0Du,
    0x0Eu,       GCR_INVALID};

uint8_t gcr_encode_nibble(uint8_t nibble)
{
    return k_gcr_encode[(unsigned)nibble & GCR_NIBBLE_MASK];
}

uint8_t gcr_decode_nibble(uint8_t five)
{
    return k_gcr_decode[(unsigned)five & GCR_FIVE_MASK];
}

void gcr_encode_byte(uint8_t value, uint8_t *hi5, uint8_t *lo5)
{
    if (hi5 != NULL)
    {
        *hi5 = gcr_encode_nibble((uint8_t)(value >> 4));
    }
    if (lo5 != NULL)
    {
        *lo5 = gcr_encode_nibble(value);
    }
}

int gcr_decode_byte(uint8_t hi5, uint8_t lo5, uint8_t *out)
{
    const uint8_t hi = gcr_decode_nibble(hi5);
    const uint8_t lo = gcr_decode_nibble(lo5);

    if (out == NULL)
    {
        return -1;
    }
    if (hi == GCR_INVALID || lo == GCR_INVALID)
    {
        return -1;
    }
    *out = (uint8_t)((uint8_t)(hi << 4) | lo);
    return 0;
}
