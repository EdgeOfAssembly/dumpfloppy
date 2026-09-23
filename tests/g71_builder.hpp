/**
 * @file g71_builder.hpp
 * @brief Encode a D71 into a GCR-1571 G71 for Catch2 tests.
 */
#ifndef DUMPFLOPPY_TEST_G71_BUILDER_HPP
#define DUMPFLOPPY_TEST_G71_BUILDER_HPP

#include "d71_builder.hpp"
#include "g64_builder.hpp"
#include "dumpfloppy/g71.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace dumpfloppy_test
{

inline constexpr uint8_t k_g71_table_tracks = dumpfloppy::k_g71_usual_tracks;
inline constexpr uint16_t k_g71_track_alloc = dumpfloppy::k_g71_usual_track_bytes;

/**
 * @brief GCR-1571 table index for a 1571 logical track (1–70).
 *
 * 168-slot images: side 0 even slots 0–82, side 1 even slots 84–166.
 * 84-slot images: side 0 indices 0–41, side 1 indices 42–83.
 */
inline uint8_t g71_table_index(uint8_t logical_track, uint8_t table_tracks)
{
    if (logical_track < 1u || logical_track > dumpfloppy::k_d71_track_count)
    {
        return 0xFFu;
    }
    if (table_tracks > 84u)
    {
        if (logical_track <= dumpfloppy::k_d64_track_count)
        {
            return static_cast<uint8_t>((logical_track - 1u) * 2u);
        }
        return static_cast<uint8_t>(
            84u + (logical_track - dumpfloppy::k_d64_track_count - 1u) * 2u);
    }
    if (logical_track <= dumpfloppy::k_d64_track_count)
    {
        return static_cast<uint8_t>(logical_track - 1u);
    }
    return static_cast<uint8_t>(42u + (logical_track - dumpfloppy::k_d64_track_count - 1u));
}

inline uint8_t g71_speed_zone(uint8_t logical_track)
{
    const uint8_t zone_track =
        (logical_track <= dumpfloppy::k_d64_track_count)
            ? logical_track
            : static_cast<uint8_t>(logical_track - dumpfloppy::k_d64_track_count);
    return g64_speed_zone(zone_track);
}

inline std::vector<uint8_t> encode_d71_track(const std::vector<uint8_t>& d71,
                                             uint8_t track, uint8_t id1,
                                             uint8_t id2)
{
    gcr_bit_writer w{};
    const uint8_t spt =
        dumpfloppy::cbm_sectors_per_track(dumpfloppy::cbm_media::d71, track);
    for (uint8_t s = 0; s < spt; ++s)
    {
        const std::size_t off =
            dumpfloppy::cbm_offset(dumpfloppy::cbm_media::d71, track, s);
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
        std::memcpy(blk + 1, d71.data() + off, dumpfloppy::k_d64_sector_bytes);
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
 * @brief Wrap a 70-track D71 in a GCR-1571 G71.
 *
 * @param[in] d71           349696-byte D71.
 * @param[in] table_tracks  168 (VICE half-tracks) or 84 (whole tracks).
 */
inline std::vector<uint8_t> d71_to_g71(const std::vector<uint8_t>& d71,
                                       uint8_t table_tracks = k_g71_table_tracks)
{
    std::vector<uint8_t> img;
    const std::size_t header = dumpfloppy::k_g71_prefix_bytes +
                               static_cast<std::size_t>(table_tracks) * 8u;
    img.assign(header, 0);
    std::memcpy(img.data(), dumpfloppy::k_g71_magic, 8);
    img[8] = dumpfloppy::k_g71_version;
    img[9] = table_tracks;
    poke_le16(img, 10, k_g71_track_alloc);

    const std::size_t bam =
        dumpfloppy::cbm_offset(dumpfloppy::cbm_media::d71, 18, 0);
    const uint8_t id1 = d71[bam + 0xA2u];
    const uint8_t id2 = d71[bam + 0xA3u];

    for (uint8_t t = 1; t <= dumpfloppy::k_d71_track_count; ++t)
    {
        const uint8_t index = g71_table_index(t, table_tracks);
        std::vector<uint8_t> gcr = encode_d71_track(d71, t, id1, id2);
        if (gcr.size() > k_g71_track_alloc)
        {
            gcr.resize(k_g71_track_alloc);
        }
        const uint32_t off = static_cast<uint32_t>(img.size());
        poke_le32(img, dumpfloppy::k_g71_prefix_bytes +
                           static_cast<std::size_t>(index) * 4u,
                  off);
        poke_le32(img,
                  dumpfloppy::k_g71_prefix_bytes +
                      static_cast<std::size_t>(table_tracks) * 4u +
                      static_cast<std::size_t>(index) * 4u,
                  g71_speed_zone(t));
        const std::size_t blob = 2u + static_cast<std::size_t>(k_g71_track_alloc);
        const std::size_t start = img.size();
        img.resize(start + blob, 0);
        poke_le16(img, start, static_cast<uint16_t>(gcr.size()));
        std::memcpy(img.data() + start + 2u, gcr.data(), gcr.size());
    }
    return img;
}

inline std::vector<uint8_t> make_sample_g71()
{
    return d71_to_g71(make_sample_d71());
}

inline std::vector<uint8_t> make_sample_g71_84()
{
    return d71_to_g71(make_sample_d71(), 84u);
}

} /* namespace dumpfloppy_test */

#endif /* DUMPFLOPPY_TEST_G71_BUILDER_HPP */
