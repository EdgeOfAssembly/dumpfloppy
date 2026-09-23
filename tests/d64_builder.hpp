/**
 * @file d64_builder.hpp
 * @brief Tiny in-memory 1541 D64 images for Catch2 tests.
 */
#ifndef DUMPFLOPPY_TEST_D64_BUILDER_HPP
#define DUMPFLOPPY_TEST_D64_BUILDER_HPP

#include "dumpfloppy/cbm.hpp"

#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace dumpfloppy_test
{

inline constexpr std::size_t k_d64_size = dumpfloppy::k_d64_35_bytes;

inline void poke_petscii(uint8_t* dest, std::size_t n, std::string_view ascii)
{
    std::memset(dest, 0xA0, n);
    const std::size_t m = ascii.size() < n ? ascii.size() : n;
    for (std::size_t i = 0; i < m; ++i)
    {
        dest[i] = static_cast<uint8_t>(ascii[i]);
    }
}

inline void bam_recount_track(uint8_t* bam, uint8_t track)
{
    uint8_t* e = bam + 4u + static_cast<std::size_t>(track - 1u) * 4u;
    const uint8_t spt = dumpfloppy::d64_sectors_per_track(track);
    uint8_t free_n = 0;
    for (uint8_t s = 0; s < spt; ++s)
    {
        const unsigned bi = static_cast<unsigned>(s) / 8u;
        const unsigned mask = 1u << (static_cast<unsigned>(s) % 8u);
        if ((e[1u + bi] & static_cast<uint8_t>(mask)) != 0u)
        {
            ++free_n;
        }
    }
    e[0] = free_n;
}

inline void bam_allocate(uint8_t* bam, uint8_t track, uint8_t sector)
{
    uint8_t* e = bam + 4u + static_cast<std::size_t>(track - 1u) * 4u;
    const unsigned bi = static_cast<unsigned>(sector) / 8u;
    const unsigned mask = 1u << (static_cast<unsigned>(sector) % 8u);
    e[1u + bi] = static_cast<uint8_t>(e[1u + bi] & static_cast<uint8_t>(~mask));
    bam_recount_track(bam, track);
}

inline void bam_init(uint8_t* bam, std::string_view name, std::string_view id)
{
    std::memset(bam, 0, dumpfloppy::k_d64_sector_bytes);
    bam[0] = 18;
    bam[1] = 1;
    bam[2] = static_cast<uint8_t>('A');
    bam[3] = 0;
    for (uint8_t t = 1; t <= dumpfloppy::k_d64_track_count; ++t)
    {
        uint8_t* e = bam + 4u + static_cast<std::size_t>(t - 1u) * 4u;
        const uint8_t spt = dumpfloppy::d64_sectors_per_track(t);
        e[0] = spt;
        e[1] = 0xFF;
        e[2] = 0xFF;
        e[3] = 0xFF;
        for (unsigned s = spt; s < 24u; ++s)
        {
            const unsigned bi = s / 8u;
            const unsigned mask = 1u << (s % 8u);
            e[1u + bi] =
                static_cast<uint8_t>(e[1u + bi] & static_cast<uint8_t>(~mask));
        }
        bam_recount_track(bam, t);
    }
    poke_petscii(bam + 0x90, 16, name);
    bam[0xA0] = 0xA0;
    bam[0xA1] = 0xA0;
    poke_petscii(bam + 0xA2, 2, id);
    bam[0xA4] = 0xA0;
    bam[0xA5] = static_cast<uint8_t>('2');
    bam[0xA6] = static_cast<uint8_t>('A');
    bam[0xA7] = 0xA0;
    bam[0xA8] = 0xA0;
    bam[0xA9] = 0xA0;
    bam[0xAA] = 0xA0;
    bam_allocate(bam, 18, 0);
    bam_allocate(bam, 18, 1);
}

inline void write_dirent(uint8_t* slot, uint8_t type, uint8_t first_t, uint8_t first_s,
                         std::string_view name, uint16_t blocks)
{
    slot[2] = type;
    slot[3] = first_t;
    slot[4] = first_s;
    poke_petscii(slot + 5, 16, name);
    slot[30] = static_cast<uint8_t>(blocks & 0xFFu);
    slot[31] = static_cast<uint8_t>((blocks >> 8) & 0xFFu);
}

/**
 * @brief Write a CBM data chain using @p media geometry (no BAM updates).
 *
 * Sequential sectors on the same track; wraps to the next track, skipping
 * 18 (D64/D71), 53 (D71), and 40 (D81). Tests do not need interleave 10.
 */
inline void write_cbm_chain(std::vector<uint8_t>& img, dumpfloppy::cbm_media media,
                            uint8_t track, uint8_t sector,
                            std::span<const uint8_t> data)
{
    auto next_data_ts = [media](uint8_t t, uint8_t s) -> std::pair<uint8_t, uint8_t>
    {
        uint8_t ns = static_cast<uint8_t>(s + 1u);
        uint8_t nt = t;
        if (ns >= dumpfloppy::cbm_sectors_per_track(media, t))
        {
            nt = static_cast<uint8_t>(t + 1u);
            ns = 0;
            if ((media == dumpfloppy::cbm_media::d64 ||
                 media == dumpfloppy::cbm_media::d71) &&
                nt == 18u)
            {
                nt = 19u;
            }
            if (media == dumpfloppy::cbm_media::d71 && nt == 53u)
            {
                nt = 54u;
            }
            if (media == dumpfloppy::cbm_media::d81 && nt == 40u)
            {
                nt = 41u;
            }
        }
        return {nt, ns};
    };

    std::size_t pos = 0;
    uint8_t t = track;
    uint8_t s = sector;
    if (data.empty())
    {
        const std::size_t off = dumpfloppy::cbm_offset(media, t, s);
        img[off] = 0;
        img[off + 1u] = 1;
        return;
    }
    while (pos < data.size())
    {
        const std::size_t remain = data.size() - pos;
        const bool last = remain <= 254u;
        const std::size_t chunk = last ? remain : 254u;
        const std::size_t off = dumpfloppy::cbm_offset(media, t, s);
        uint8_t* sec = img.data() + off;
        if (last)
        {
            sec[0] = 0;
            sec[1] = static_cast<uint8_t>(1u + chunk);
            std::memcpy(sec + 2, data.data() + pos, chunk);
            return;
        }
        const auto [nt, ns] = next_data_ts(t, s);
        sec[0] = nt;
        sec[1] = ns;
        std::memcpy(sec + 2, data.data() + pos, 254u);
        pos += 254u;
        t = nt;
        s = ns;
    }
}

/**
 * @brief Write a CBM data chain starting at @p track/@p sector.
 *
 * Uses sequential sectors on the same track (tests do not need interleave 10).
 */
inline void write_chain(std::vector<uint8_t>& img, uint8_t track, uint8_t sector,
                        std::span<const uint8_t> data, uint8_t* bam)
{
    std::size_t pos = 0;
    uint8_t t = track;
    uint8_t s = sector;
    if (data.empty())
    {
        const std::size_t off = dumpfloppy::d64_offset(t, s);
        img[off] = 0;
        img[off + 1u] = 1;
        if (bam != nullptr)
        {
            bam_allocate(bam, t, s);
        }
        return;
    }
    while (pos < data.size())
    {
        const std::size_t remain = data.size() - pos;
        const bool last = remain <= 254u;
        const std::size_t chunk = last ? remain : 254u;
        const std::size_t off = dumpfloppy::d64_offset(t, s);
        uint8_t* sec = img.data() + off;
        if (bam != nullptr)
        {
            bam_allocate(bam, t, s);
        }
        if (last)
        {
            sec[0] = 0;
            sec[1] = static_cast<uint8_t>(1u + chunk);
            std::memcpy(sec + 2, data.data() + pos, chunk);
            return;
        }
        uint8_t nt = t;
        uint8_t ns = static_cast<uint8_t>(s + 1u);
        if (ns >= dumpfloppy::d64_sectors_per_track(t))
        {
            nt = static_cast<uint8_t>(t + 1u);
            ns = 0;
            if (nt == 18u)
            {
                nt = 19u;
            }
        }
        sec[0] = nt;
        sec[1] = ns;
        std::memcpy(sec + 2, data.data() + pos, 254u);
        pos += 254u;
        t = nt;
        s = ns;
    }
}

/** @brief Distinctive 300-byte PRG payload (254 + 46, two sectors). */
inline std::vector<uint8_t> sample_prg_bytes()
{
    std::vector<uint8_t> p(300);
    for (std::size_t i = 0; i < p.size(); ++i)
    {
        p[i] = static_cast<uint8_t>(i & 0xFFu);
    }
    const char tag[] = "HELLO-PRG";
    std::memcpy(p.data(), tag, sizeof(tag) - 1u);
    return p;
}

/** @brief Short deleted SEQ payload. */
inline std::vector<uint8_t> sample_seq_bytes()
{
    const char tag[] = "SEQ-DATA";
    return std::vector<uint8_t>(tag, tag + sizeof(tag) - 1u);
}

/**
 * @brief 174848-byte D64 with BAM, one closed PRG, and one deleted SEQ.
 *
 * Disk name `TEST DISK`, ID `DF`. PRG `HELLO` at 17/0; SEQ `OLDSEQ` at 17/2.
 */
inline std::vector<uint8_t> make_sample_d64()
{
    std::vector<uint8_t> img(k_d64_size, 0);
    uint8_t* bam = img.data() + dumpfloppy::d64_offset(18, 0);
    bam_init(bam, "TEST DISK", "DF");

    const std::vector<uint8_t> prg = sample_prg_bytes();
    const std::vector<uint8_t> seq = sample_seq_bytes();
    write_chain(img, 17, 0, prg, bam);
    write_chain(img, 17, 2, seq, bam);

    uint8_t* dir = img.data() + dumpfloppy::d64_offset(18, 1);
    dir[0] = 0;
    dir[1] = 0xFF;
    write_dirent(dir, 0x82, 17, 0, "HELLO", 2);
    write_dirent(dir + 32, 0x01, 17, 2, "OLDSEQ", 1);
    return img;
}

} /* namespace dumpfloppy_test */

#endif /* DUMPFLOPPY_TEST_D64_BUILDER_HPP */
