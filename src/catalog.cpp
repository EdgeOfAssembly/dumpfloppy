/**
 * @file catalog.cpp
 * @brief Whole-image XXH64 → title + copy-protection label.
 *
 * Each dump file has its own hash (.mfm ≠ .86f of the same disk).
 */
#include "dumpfloppy/catalog.hpp"

#include <array>

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
constexpr std::array<row, 2> k_disks{{
    {"93a5a1a9002057dd",
     "Commando (Data East USA, 1986) 180K 5.25\" SS/DD booter",
     "long sector C39:H0:S7 1024 + intentional DAM CRC; boot INT 13h waits for AH=10h"},
    {"50b2697bd3ff5a9f",
     "Commando (Data East USA, 1986) 180K 5.25\" SS/DD booter",
     "long sector C39:H0:S7 1024 + intentional DAM CRC; boot INT 13h waits for AH=10h"},
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
