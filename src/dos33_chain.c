/**
 * @file dos33_chain.c
 * @brief Bounded DOS 3.3 T/S list chain walk. Production callers must use this.
 */
#include "dumpfloppy/dos33_chain.h"

int dos33_ts_in_range(uint8_t t, uint8_t s, uint8_t tracks, uint8_t spt)
{
    return t != 0u && t < tracks && s < spt;
}

enum dos33_chain_status dos33_ts_chain(const uint8_t* vol, size_t vol_len, uint8_t tracks,
                                       uint8_t spt, uint8_t t, uint8_t s, uint8_t* out_t,
                                       uint8_t* out_s, size_t cap, size_t* n)
{
    *n = 0u;
    if (vol == NULL || out_t == NULL || out_s == NULL)
    {
        return DOS33_CHAIN_BAD_LINK;
    }
    while (t != 0u)
    {
        if (!dos33_ts_in_range(t, s, tracks, spt))
        {
            return DOS33_CHAIN_BAD_LINK;
        }
        const size_t off = ((size_t)t * spt + s) * DOS33_CHAIN_SECTOR;
        if (off > vol_len || vol_len - off < DOS33_CHAIN_SECTOR)
        {
            return DOS33_CHAIN_BAD_LINK;
        }
        for (size_t i = 0u; i < *n; ++i)
        {
            if (out_t[i] == t && out_s[i] == s)
            {
                return DOS33_CHAIN_LOOP;
            }
        }
        if (*n >= cap)
        {
            return DOS33_CHAIN_TOO_LONG;
        }
        out_t[*n] = t;
        out_s[*n] = s;
        *n += 1u;
        t = vol[off + 1u];
        s = vol[off + 2u];
    }
    return DOS33_CHAIN_OK;
}
