/**
 * @file geometry.cpp
 * @brief Standard IBM PC floppy sizes and media-byte names.
 */
#include "dumpfloppy/geometry.hpp"
#include "dumpfloppy/amiga.hpp"
#include "dumpfloppy/cbm.hpp"
#include "dumpfloppy/util.hpp"

#include <array>
#include <cstdio>

namespace dumpfloppy
{
namespace
{

struct size_row
{
    uint64_t bytes;
    uint32_t cyl;
    uint32_t heads;
    uint32_t spt;
    uint32_t bps;
    const char* name;
};

/* Classic raw images; 81/82-track overdumps are handled as trailing bytes. */
constexpr std::array<size_row, 12> k_sizes{{
    {163840u, 40, 1, 8, 512, "5.25\" 160K SS/DD"},
    {184320u, 40, 1, 9, 512, "5.25\" 180K SS/DD"},
    {327680u, 40, 2, 8, 512, "5.25\" 320K DS/DD"},
    {368640u, 40, 2, 9, 512, "5.25\" 360K DS/DD"},
    {737280u, 80, 2, 9, 512, "3.5\" 720K DS/DD"},
    {1228800u, 80, 2, 15, 512, "5.25\" 1.2M DS/HD"},
    {1474560u, 80, 2, 18, 512, "3.5\" 1.44M DS/HD"},
    {1720320u, 80, 2, 21, 512, "3.5\" 1.68M DMF"},
    {1763328u, 82, 2, 21, 512, "3.5\" 1.72M DMF (82 track)"},
    {2949120u, 80, 2, 36, 512, "3.5\" 2.88M DS/ED"},
    {819200u, 80, 2, 10, 512, "3.5\" 800K (10 spt)"},
    {746496u, 81, 2, 9, 512, "3.5\" 720K 81-track overdump"},
}};

} /* namespace */

geometry geometry_from_size(uint64_t byte_count)
{
    geometry g{};
    g.bytes_per_sector = 512;
    /* CBM and ADF sizes that would otherwise look like IBM + trailer.
       D81 819200 collides with IBM 800K (10 spt); leave that to analyse
       (valid CBMFS header) so FAT 800K images stay IBM. */
    if (byte_count == k_d64_35_bytes)
    {
        g.bytes_per_sector = k_d64_sector_bytes;
        g.expected_bytes = k_d64_35_bytes;
        g.media_name = "Commodore 1541 35-track D64";
        return g;
    }
    if (byte_count == k_d64_35_error_bytes)
    {
        g.bytes_per_sector = k_d64_sector_bytes;
        g.expected_bytes = k_d64_35_bytes;
        g.media_name = "Commodore 1541 35-track D64 + error map";
        return g;
    }
    if (byte_count == k_d71_bytes)
    {
        g.bytes_per_sector = k_d64_sector_bytes;
        g.expected_bytes = k_d71_bytes;
        g.media_name = "Commodore 1571 70-track D71";
        return g;
    }
    if (byte_count == k_d71_error_bytes)
    {
        g.bytes_per_sector = k_d64_sector_bytes;
        g.expected_bytes = k_d71_bytes;
        g.media_name = "Commodore 1571 70-track D71 + error map";
        return g;
    }
    if (byte_count == k_d81_error_bytes)
    {
        g.bytes_per_sector = k_d64_sector_bytes;
        g.cylinders = 80;
        g.heads = 1;
        g.sectors_per_track = 40;
        g.expected_bytes = k_d81_bytes;
        g.media_name = "Commodore 1581 80-track D81 + error map";
        return g;
    }
    if (byte_count == k_adf_dd_bytes)
    {
        g.cylinders = 80;
        g.heads = 2;
        g.sectors_per_track = 11;
        g.bytes_per_sector = k_adf_sector_bytes;
        g.expected_bytes = k_adf_dd_bytes;
        g.media_name = "Amiga DD ADF (80×2×11×512)";
        return g;
    }
    if (byte_count == k_adf_hd_bytes)
    {
        g.cylinders = 80;
        g.heads = 2;
        g.sectors_per_track = 22;
        g.bytes_per_sector = k_adf_sector_bytes;
        g.expected_bytes = k_adf_hd_bytes;
        g.media_name = "Amiga HD ADF (80×2×22×512)";
        return g;
    }
    for (const size_row& row : k_sizes)
    {
        if (row.bytes == byte_count)
        {
            g.cylinders = row.cyl;
            g.heads = row.heads;
            g.sectors_per_track = row.spt;
            g.bytes_per_sector = row.bps;
            g.expected_bytes = row.bytes;
            g.media_name = row.name;
            return g;
        }
    }
    /* Closest smaller standard size → remainder is trailer/overdump. */
    const size_row* best = nullptr;
    for (const size_row& row : k_sizes)
    {
        if (row.bytes < byte_count)
        {
            if (best == nullptr || row.bytes > best->bytes)
            {
                best = &row;
            }
        }
    }
    if (best != nullptr && byte_count - best->bytes < best->bytes / 10u)
    {
        g.cylinders = best->cyl;
        g.heads = best->heads;
        g.sectors_per_track = best->spt;
        g.bytes_per_sector = best->bps;
        g.expected_bytes = best->bytes;
        g.media_name = std::string(best->name) + " + trailer";
        return g;
    }
    g.media_name = "non-standard size";
    return g;
}

std::string media_descriptor_name(uint8_t media)
{
    switch (media)
    {
        case 0xF0:
            return "3.5\" 1.44M/2.88M (F0)";
        case 0xF8:
            return "fixed disk (F8)";
        case 0xF9:
            return "3.5\" 720K or 5.25\" 1.2M (F9)";
        case 0xFA:
            return "5.25\" 320K? / NEC (FA)";
        case 0xFB:
            return "3.5\" 640K (FB)";
        case 0xFC:
            return "5.25\" 180K SS (FC)";
        case 0xFD:
            return "5.25\" 360K DS (FD)";
        case 0xFE:
            return "5.25\" 160K SS (FE)";
        case 0xFF:
            return "5.25\" 320K DS (FF)";
        default:
        {
            char buf[32] = {};
            std::snprintf(buf, sizeof(buf), "unknown (0x%02X)", media);
            return buf;
        }
    }
}

container_kind container_from_path(const std::string& path)
{
    const std::string lower = ascii_lower(path);
    if (lower.size() >= 4 && lower.ends_with(".ima"))
    {
        return container_kind::ima_winimage;
    }
    if (lower.size() >= 4 && lower.ends_with(".img"))
    {
        return container_kind::img_raw;
    }
    if (lower.size() >= 4 && lower.ends_with(".mfm"))
    {
        return container_kind::hxc_mfm;
    }
    if (lower.size() >= 4 && lower.ends_with(".86f"))
    {
        return container_kind::box86f;
    }
    if (lower.size() >= 4 && lower.ends_with(".d64"))
    {
        return container_kind::d64_c64;
    }
    if (lower.size() >= 4 && lower.ends_with(".d71"))
    {
        return container_kind::d71_c64;
    }
    if (lower.size() >= 4 && lower.ends_with(".d81"))
    {
        return container_kind::d81_c64;
    }
    if (lower.size() >= 4 && lower.ends_with(".adf"))
    {
        return container_kind::adf_amiga;
    }
    if (lower.size() >= 4 && lower.ends_with(".g64"))
    {
        return container_kind::g64_c64;
    }
    if (lower.size() >= 4 && lower.ends_with(".trd"))
    {
        return container_kind::trd_spectrum;
    }
    if (lower.size() >= 4 && lower.ends_with(".ipf"))
    {
        return container_kind::ipf_sps;
    }
    if (lower.size() >= 4 && lower.ends_with(".woz"))
    {
        return container_kind::woz_apple;
    }
    if (lower.size() >= 4 && lower.ends_with(".stx"))
    {
        return container_kind::stx_atari;
    }
    if (lower.size() >= 4 && lower.ends_with(".2mg"))
    {
        return container_kind::img2mg_apple;
    }
    return container_kind::unknown_raw;
}

} /* namespace dumpfloppy */
