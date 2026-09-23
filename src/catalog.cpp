/**
 * @file catalog.cpp
 * @brief Whole-image XXH64 → title + copy-protection label.
 *
 * Each dump file has its own hash (.mfm ≠ .86f of the same disk).
 * Game images stay local under /mnt/dumpfloppy-fixtures/ (never GitHub).
 */
#include "dumpfloppy/catalog.hpp"

#include <array>
#include <string_view>

namespace dumpfloppy
{
namespace
{

struct row
{
    const char* xxh64;
    const char* title;
    const char* protection;
};

/* Hashes are of the entire container file (seed-0 XXH64). */
constexpr std::array<row, 22> k_disks{{
    /* IBM PC flux / IMA */
    {"93a5a1a9002057dd",
     "Commando (Data East USA, 1986) 180K 5.25\" SS/DD booter",
     "long sector C39:H0:S7 1024 + intentional DAM CRC; boot INT 13h waits for AH=10h"},
    {"50b2697bd3ff5a9f",
     "Commando (Data East USA, 1986) 180K 5.25\" SS/DD booter",
     "long sector C39:H0:S7 1024 + intentional DAM CRC; boot INT 13h waits for AH=10h"},
    {"2091da8694d943f3",
     "Batman - The Caped Crusader (Data East USA, 1989) 360K Disk 1",
     "HLS Duplication: nonstandard sector IDs C39:H0:S241, C39:H0:S222, C39:H1:S45 (256)"},
    {"2e3ff2027c357397",
     "Batman - The Caped Crusader (Data East USA, 1989) 360K Disk 1",
     "HLS Duplication: nonstandard sector IDs C39:H0:S241, C39:H0:S222, C39:H1:S45 (256)"},
    {"334b6a1ad6596712",
     "Batman - The Caped Crusader (Data East USA, 1989) 360K Disk 2",
     "HLS Duplication: nonstandard sector IDs C39:H0:S241, C39:H0:S222, C39:H1:S45 (256)"},
    {"9f3799065d75aad8",
     "Batman - The Caped Crusader (Data East USA, 1989) 360K Disk 2",
     "HLS Duplication: nonstandard sector IDs C39:H0:S241, C39:H0:S222, C39:H1:S45 (256)"},
    {"a4cb00f350c51e63",
     "Populous (Electronic Arts / Bullfrog, 1989) 360K",
     "none (standard 360K FAT12)"},
    {"bd833a724a08fbc5",
     "2400 A.D. (ORIGIN Systems, 1988) 360K HxC MFM",
     "long sectors C6:H0:S2-S5 1024 + HLS-style ID C6:H0:S170 512 (Origin)"},
    {"6f25dadaa3091031",
     "2400 A.D. (ORIGIN Systems, 1988) 360K 86F",
     "long sectors C6:H0:S2-S5 1024 + HLS-style ID C6:H0:S170 512 (Origin; decode via .mfm)"},
    {"7d40562d1f7fee2e",
     "2400 A.D. (ORIGIN Systems, 1988) 360K cracked IMA",
     "cracked ([cr] IMA); Origin long-sector/HLS key disk removed — standard 360K FAT12"},

    /* Commodore 1541 D64 / G64 */
    {"be2324a14653936b",
     "The Last Ninja (System 3, 1987) Side A D64",
     "Paranoid (System 3): disk PARANO, DOS type D, every dir entry chains 18/9 "
     "(PARA-PROTECT + deleted PROTECTED BY PAR)"},
    {"00b6c60995d66466",
     "The Last Ninja (System 3, 1987) Side B D64",
     "Paranoid (System 3): disk PARANO, DOS type D, SIDE (2); every dir entry chains 18/9"},
    {"60ac8224c2c21c17",
     "The Last Ninja (System 3, 1987) combined/cracked D64",
     "cracked (Paranoid removed); single PRG THE LAST NINJA"},
    {"52f291e48407359e",
     "The Last Ninja (System 3, 1987) TLG trainer D64",
     "cracked (TLG trainer); Paranoid removed — disk DIGITAL DUNGEON"},
    {"e3ce744e13387bd8",
     "Archon (Electronic Arts, 1983) 1541 G64",
     "Electronic Arts custom DOS (LOAD\"EA\",8,1); GCR half-track 34.5 + extra track 41"},
    {"752b75a5b8a31147",
     "Batman - The Caped Crusader (Ocean, 1988) 1541 G64",
     "Ocean loader: GCR extra track 36 (beyond 1541 35-track CBMFS)"},
    {"1e1624668e2ba983",
     "Boulder Dash (First Star, 1984) D64",
     "none (standard 1541 CBMFS)"},
    {"8955f9748027eef3",
     "Impossible Mission (Epyx, 1984) REM crack D64",
     "cracked (REM); Rapidlok removed — D64 is not the original Epyx key disk"},
    {"97b86110caded04f",
     "Karateka (Jordan Mechner copy, 1985-05-02) D64",
     "none (standard 1541 CBMFS)"},

    /* Commodore 1581 */
    {"5880b6fd586c7dc0",
     "OMEGA-Q V2.1 (C128) D81",
     "none (standard 1581 CBMFS)"},

    /* Amiga ADF */
    {"b28e391eadbc8a56",
     "Beast Sonix (Scoopex, 1990) ADF",
     "none (OFS music disk)"},
    {"0f4c8a1defd7770e",
     "Lemmings demo (Skid Row, 1990) ADF",
     "none (Skid Row 1990 demo; OFS)"},
}};

} /* namespace */

catalog_hit catalog_lookup(std::string_view xxh64_hex)
{
    catalog_hit hit{};
    for (const row& r : k_disks)
    {
        if (xxh64_hex == r.xxh64)
        {
            hit.found = true;
            hit.title = r.title;
            hit.protection = r.protection;
            return hit;
        }
    }
    return hit;
}

} /* namespace dumpfloppy */
