/**
 * @file g64_builder.hpp
 * @brief Encode a D64 into a GCR-1541 G64 for Catch2 tests.
 */
#ifndef DUMPFLOPPY_TEST_G64_BUILDER_HPP
#define DUMPFLOPPY_TEST_G64_BUILDER_HPP

#include "d64_builder.hpp"
#include "dumpfloppy/g64.hpp"
#include "dumpfloppy/gcr_codec.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace dumpfloppy_test
{

inline constexpr uint8_t k_g64_table_tracks = dumpfloppy::k_g64_usual_tracks;
inline constexpr uint16_t k_g64_track_alloc = dumpfloppy::k_g64_usual_track_bytes;

struct gcr_bit_writer
{
    std::vector<uint8_t> bytes{};
    uint8_t acc = 0;
    unsigned filled = 0;

    void put_bit(unsigned bit)
    {
        acc = static_cast<uint8_t>(static_cast<uint8_t>(acc << 1) | (bit & 1u));
        ++filled;
        if (filled == 8u)
        {
            bytes.push_back(acc);
            acc = 0;
            filled = 0;
        }
    }

    void put_bits_msb(uint8_t value, unsigned n)
    {
        for (unsigned i = 0; i < n; ++i)
        {
            const unsigned shift = n - 1u - i;
            put_bit(static_cast<unsigned>((value >> shift) & 1u));
        }
    }

    void put_gcr_byte(uint8_t value)
    {
        uint8_t hi = 0;
        uint8_t lo = 0;
        gcr_encode_byte(value, &hi, &lo);
        put_bits_msb(hi, 5);
        put_bits_msb(lo, 5);
    }

    void put_raw_byte(uint8_t value)
    {
        put_bits_msb(value, 8);
    }

    void put_sync()
    {
        for (unsigned i = 0; i < 40u; ++i)
        {
            put_bit(1u);
        }
    }

    void pad_to_byte()
    {
        while (filled != 0u)
        {
            put_bit(0u);
        }
    }
};

inline void poke_le16(std::vector<uint8_t>& img, std::size_t off, uint16_t v)
{
    img[off] = static_cast<uint8_t>(v & 0xFFu);
    img[off + 1u] = static_cast<uint8_t>((v >> 8) & 0xFFu);
}

inline void poke_le32(std::vector<uint8_t>& img, std::size_t off, uint32_t v)
{
    img[off] = static_cast<uint8_t>(v & 0xFFu);
    img[off + 1u] = static_cast<uint8_t>((v >> 8) & 0xFFu);
    img[off + 2u] = static_cast<uint8_t>((v >> 16) & 0xFFu);
    img[off + 3u] = static_cast<uint8_t>((v >> 24) & 0xFFu);
}

inline uint8_t g64_speed_zone(uint8_t track)
{
    if (track <= 17u)
    {
        return 3u;
    }
    if (track <= 24u)
    {
        return 2u;
    }
    if (track <= 30u)
    {
        return 1u;
    }
    return 0u;
}

inline std::vector<uint8_t> encode_d64_track(const std::vector<uint8_t>& d64,
                                             uint8_t track, uint8_t id1,
                                             uint8_t id2)
{
    gcr_bit_writer w{};
    const uint8_t spt = dumpfloppy::d64_sectors_per_track(track);
    for (uint8_t s = 0; s < spt; ++s)
    {
        const std::size_t off = dumpfloppy::d64_offset(track, s);
        w.put_sync();
        uint8_t hdr[8] = {0x08, 0, s, track, id2, id1, 0x0F, 0x0F};
        hdr[1] = static_cast<uint8_t>(s ^ track ^ id2 ^ id1);
        for (uint8_t b : hdr)
        {
            w.put_gcr_byte(b);
        }
        for (unsigned g = 0; g < 9u; ++g)
        {
            w.put_raw_byte(0x55);
        }
        w.put_sync();
        uint8_t blk[260] = {};
        blk[0] = 0x07;
        std::memcpy(blk + 1, d64.data() + off, dumpfloppy::k_d64_sector_bytes);
        uint8_t csum = 0;
        for (unsigned i = 1; i <= 256u; ++i)
        {
            csum = static_cast<uint8_t>(csum ^ blk[i]);
        }
        blk[257] = csum;
        for (uint8_t b : blk)
        {
            w.put_gcr_byte(b);
        }
        for (unsigned g = 0; g < 8u; ++g)
        {
            w.put_raw_byte(0x55);
        }
    }
    w.pad_to_byte();
    return w.bytes;
}

/**
 * @brief Wrap a 35-track D64 in a standard 84-slot GCR-1541 G64.
 *
 * Whole tracks 1–35 are stored; half-tracks and 36–42 are empty offsets.
 */
inline std::vector<uint8_t> d64_to_g64(const std::vector<uint8_t>& d64)
{
    std::vector<uint8_t> img;
    const std::size_t header = dumpfloppy::k_g64_prefix_bytes +
                               static_cast<std::size_t>(k_g64_table_tracks) * 8u;
    img.assign(header, 0);
    std::memcpy(img.data(), dumpfloppy::k_g64_magic, 8);
    img[8] = dumpfloppy::k_g64_version;
    img[9] = k_g64_table_tracks;
    poke_le16(img, 10, k_g64_track_alloc);

    const uint8_t id1 = d64[dumpfloppy::d64_offset(18, 0) + 0xA2u];
    const uint8_t id2 = d64[dumpfloppy::d64_offset(18, 0) + 0xA3u];

    for (uint8_t t = 1; t <= dumpfloppy::k_d64_track_count; ++t)
    {
        const uint8_t index = static_cast<uint8_t>((t - 1u) * 2u);
        std::vector<uint8_t> gcr = encode_d64_track(d64, t, id1, id2);
        if (gcr.size() > k_g64_track_alloc)
        {
            gcr.resize(k_g64_track_alloc);
        }
        const uint32_t off = static_cast<uint32_t>(img.size());
        poke_le32(img, dumpfloppy::k_g64_prefix_bytes +
                           static_cast<std::size_t>(index) * 4u,
                  off);
        poke_le32(img,
                  dumpfloppy::k_g64_prefix_bytes +
                      static_cast<std::size_t>(k_g64_table_tracks) * 4u +
                      static_cast<std::size_t>(index) * 4u,
                  g64_speed_zone(t));
        const std::size_t blob = 2u + static_cast<std::size_t>(k_g64_track_alloc);
        const std::size_t start = img.size();
        img.resize(start + blob, 0);
        poke_le16(img, start, static_cast<uint16_t>(gcr.size()));
        std::memcpy(img.data() + start + 2u, gcr.data(), gcr.size());
    }
    return img;
}

inline std::vector<uint8_t> make_sample_g64()
{
    return d64_to_g64(make_sample_d64());
}

} /* namespace dumpfloppy_test */

#endif /* DUMPFLOPPY_TEST_G64_BUILDER_HPP */
