/**
 * @file d71_builder.hpp
 * @brief Tiny in-memory 1571 D71 images for Catch2 tests.
 */
#ifndef DUMPFLOPPY_TEST_D71_BUILDER_HPP
#define DUMPFLOPPY_TEST_D71_BUILDER_HPP

#include "d64_builder.hpp"
#include "dumpfloppy/cbm.hpp"

#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>

namespace dumpfloppy_test
{

inline constexpr std::size_t k_d71_size = dumpfloppy::k_d71_bytes;

inline void d71_bam_side1_init(uint8_t* bam53)
{
    std::memset(bam53, 0, dumpfloppy::k_d64_sector_bytes);
    for (uint8_t t = 36; t <= dumpfloppy::k_d71_track_count; ++t)
    {
        uint8_t* e = bam53 + static_cast<std::size_t>(t - 36u) * 3u;
        const uint8_t spt =
            dumpfloppy::cbm_sectors_per_track(dumpfloppy::cbm_media::d71, t);
        e[0] = 0xFF;
        e[1] = 0xFF;
        e[2] = 0xFF;
        for (unsigned s = spt; s < 24u; ++s)
        {
            const unsigned bi = s / 8u;
            const unsigned mask = 1u << (s % 8u);
            e[bi] = static_cast<uint8_t>(e[bi] & static_cast<uint8_t>(~mask));
        }
    }
}

inline void d71_bam_allocate(uint8_t* bam18, uint8_t* bam53, uint8_t track,
                             uint8_t sector)
{
    if (track <= dumpfloppy::k_d64_track_count)
    {
        bam_allocate(bam18, track, sector);
        return;
    }
    uint8_t* e = bam53 + static_cast<std::size_t>(track - 36u) * 3u;
    const unsigned bi = static_cast<unsigned>(sector) / 8u;
    const unsigned mask = 1u << (static_cast<unsigned>(sector) % 8u);
    e[bi] = static_cast<uint8_t>(e[bi] & static_cast<uint8_t>(~mask));
    uint8_t& free_n = bam18[0xDD + static_cast<unsigned>(track - 36u)];
    if (free_n > 0u)
    {
        --free_n;
    }
}

inline void d71_bam_init(uint8_t* bam18, uint8_t* bam53, std::string_view name,
                         std::string_view id)
{
    bam_init(bam18, name, id);
    bam18[3] = 0x80; /* 1571 double-sided */
    for (uint8_t t = 36; t <= dumpfloppy::k_d71_track_count; ++t)
    {
        bam18[0xDD + static_cast<unsigned>(t - 36u)] =
            dumpfloppy::cbm_sectors_per_track(dumpfloppy::cbm_media::d71, t);
    }
    d71_bam_side1_init(bam53);
    d71_bam_allocate(bam18, bam53, 53, 0);
}

/** @brief Distinctive 300-byte PRG stored on D71 side 1 (track 36). */
inline std::vector<uint8_t> sample_d71_prg_bytes()
{
    std::vector<uint8_t> p(300);
    for (std::size_t i = 0; i < p.size(); ++i)
    {
        p[i] = static_cast<uint8_t>((i + 0x36u) & 0xFFu);
    }
    const char tag[] = "D71-SIDE1";
    std::memcpy(p.data(), tag, sizeof(tag) - 1u);
    return p;
}

/**
 * @brief 349696-byte D71 with BAM 18/0 and one closed PRG on track 36.
 *
 * Disk name `TEST 1571`, ID `71`. PRG `SIDE1` at 36/0 (two sectors).
 */
inline std::vector<uint8_t> make_sample_d71()
{
    std::vector<uint8_t> img(k_d71_size, 0);
    uint8_t* bam18 = img.data() + dumpfloppy::cbm_offset(dumpfloppy::cbm_media::d71,
                                                         18, 0);
    uint8_t* bam53 = img.data() + dumpfloppy::cbm_offset(dumpfloppy::cbm_media::d71,
                                                         53, 0);
    d71_bam_init(bam18, bam53, "TEST 1571", "71");

    const std::vector<uint8_t> prg = sample_d71_prg_bytes();
    write_cbm_chain(img, dumpfloppy::cbm_media::d71, 36, 0, prg);
    d71_bam_allocate(bam18, bam53, 36, 0);
    d71_bam_allocate(bam18, bam53, 36, 1);

    uint8_t* dir = img.data() + dumpfloppy::cbm_offset(dumpfloppy::cbm_media::d71,
                                                      18, 1);
    dir[0] = 0;
    dir[1] = 0xFF;
    write_dirent(dir, 0x82, 36, 0, "SIDE1", 2);
    return img;
}

} /* namespace dumpfloppy_test */

#endif /* DUMPFLOPPY_TEST_D71_BUILDER_HPP */
