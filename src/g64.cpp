/**
 * @file g64.cpp
 * @brief GCR-1541 container parse and 1541 GCR track → D64 sector decode.
 *
 * Layout and sector GCR follow Peter Schepers `G64.TXT` (signature,
 * 84-slot half-track table, SYNC ≥10 ones, header ID $08, data ID $07).
 */
#include "dumpfloppy/g64.hpp"
#include "dumpfloppy/gcr_codec.h"
#include "dumpfloppy/util.hpp"

#include <cstring>
#include <vector>

namespace dumpfloppy
{
namespace
{

constexpr uint8_t k_sync_ones = 10u;
constexpr uint8_t k_header_id = 0x08u;
constexpr uint8_t k_data_id = 0x07u;
constexpr unsigned k_header_bytes = 8u;
constexpr unsigned k_data_block_bytes = 260u;
constexpr uint16_t k_max_track_alloc = 32768u;

struct g64_header
{
    bool ok = false;
    uint8_t track_count = 0;
    uint16_t max_track_bytes = 0;
};

[[nodiscard]] g64_header read_g64_header(std::span<const uint8_t> image)
{
    g64_header h{};
    if (image.size() < k_g64_prefix_bytes)
    {
        return h;
    }
    if (std::memcmp(image.data(), k_g64_magic, 8) != 0)
    {
        return h;
    }
    if (image[8] != k_g64_version)
    {
        return h;
    }
    const uint8_t n = image[9];
    if (n == 0u || n > k_g64_max_tracks)
    {
        return h;
    }
    const uint16_t max_bytes = read_le16(image, 10);
    if (max_bytes == 0u || max_bytes > k_max_track_alloc)
    {
        return h;
    }
    const std::size_t table_bytes =
        k_g64_prefix_bytes + (static_cast<std::size_t>(n) * 8u);
    if (image.size() < table_bytes)
    {
        return h;
    }
    h.ok = true;
    h.track_count = n;
    h.max_track_bytes = max_bytes;
    return h;
}

[[nodiscard]] bool track_bit(std::span<const uint8_t> bytes, std::size_t nbits,
                             std::size_t i) noexcept
{
    i %= nbits;
    const std::size_t by = i / 8u;
    const unsigned shift = 7u - static_cast<unsigned>(i % 8u);
    return ((bytes[by] >> shift) & 1u) != 0u;
}

[[nodiscard]] uint8_t take5(std::span<const uint8_t> bytes, std::size_t nbits,
                            std::size_t& bit) noexcept
{
    uint8_t v = 0;
    for (unsigned n = 0; n < 5u; ++n)
    {
        v = static_cast<uint8_t>(static_cast<uint8_t>(v << 1) |
                                 (track_bit(bytes, nbits, bit) ? 1u : 0u));
        ++bit;
    }
    return v;
}

bool decode_gcr_bytes(std::span<const uint8_t> bytes, std::size_t nbits,
                      std::size_t& bit, uint8_t* out, std::size_t n)
{
    for (std::size_t i = 0; i < n; ++i)
    {
        const uint8_t hi = take5(bytes, nbits, bit);
        const uint8_t lo = take5(bytes, nbits, bit);
        if (gcr_decode_byte(hi, lo, out + i) != 0)
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief First payload bit after a run of ≥10 ones, or @p scan_end if none.
 *
 * @p from is a linear index (may exceed @p nbits); bits wrap via modulo.
 */
[[nodiscard]] std::size_t find_sync_payload(std::span<const uint8_t> bytes,
                                            std::size_t nbits, std::size_t from,
                                            std::size_t scan_end) noexcept
{
    unsigned ones = 0;
    for (std::size_t k = from; k < scan_end; ++k)
    {
        if (track_bit(bytes, nbits, k))
        {
            if (ones < 255u)
            {
                ++ones;
            }
        }
        else
        {
            if (ones >= k_sync_ones)
            {
                return k;
            }
            ones = 0;
        }
    }
    return scan_end;
}

void store_sector(std::vector<uint8_t>& d64, std::vector<uint8_t>& have_crc,
                  uint8_t track, uint8_t sector, const uint8_t* data, bool crc_ok)
{
    if (!d64_ts_valid(track, sector))
    {
        return;
    }
    const std::size_t off = d64_offset(track, sector);
    const std::size_t idx = off / static_cast<std::size_t>(k_d64_sector_bytes);
    if (have_crc[idx] != 0u)
    {
        return;
    }
    std::memcpy(d64.data() + off, data, k_d64_sector_bytes);
    if (crc_ok)
    {
        have_crc[idx] = 1u;
    }
}

[[nodiscard]] uint8_t xor_range(const uint8_t* p, std::size_t n) noexcept
{
    uint8_t x = 0;
    for (std::size_t i = 0; i < n; ++i)
    {
        x = static_cast<uint8_t>(x ^ p[i]);
    }
    return x;
}

void decode_gcr_track(std::span<const uint8_t> gcr, std::vector<uint8_t>& d64,
                      std::vector<uint8_t>& have_crc)
{
    if (gcr.empty())
    {
        return;
    }
    const std::size_t nbits = gcr.size() * 8u;
    const std::size_t scan_end = nbits * 2u;
    std::size_t i = 0;
    bool have_header = false;
    uint8_t hdr_track = 0;
    uint8_t hdr_sector = 0;

    while (i < scan_end)
    {
        const std::size_t payload = find_sync_payload(gcr, nbits, i, scan_end);
        if (payload >= scan_end)
        {
            break;
        }

        uint8_t hdr[k_header_bytes] = {};
        std::size_t bit = payload;
        if (decode_gcr_bytes(gcr, nbits, bit, hdr, k_header_bytes) &&
            hdr[0] == k_header_id)
        {
            hdr_sector = hdr[2];
            hdr_track = hdr[3];
            have_header = true;
            i = bit;
            continue;
        }

        if (have_header)
        {
            uint8_t blk[k_data_block_bytes] = {};
            bit = payload;
            if (decode_gcr_bytes(gcr, nbits, bit, blk, k_data_block_bytes) &&
                blk[0] == k_data_id)
            {
                const uint8_t expect = xor_range(blk + 1, k_d64_sector_bytes);
                const bool crc_ok = (blk[257] == expect);
                store_sector(d64, have_crc, hdr_track, hdr_sector, blk + 1, crc_ok);
                have_header = false;
                i = bit;
                continue;
            }
        }

        i = payload + 1u;
    }
}

[[nodiscard]] uint8_t table_track_number(uint8_t track_count, uint8_t index) noexcept
{
    if (track_count > 42u)
    {
        if ((index % 2u) != 0u)
        {
            return 0u;
        }
        return static_cast<uint8_t>(index / 2u + 1u);
    }
    return static_cast<uint8_t>(index + 1u);
}

} /* namespace */

bool is_g64_image(std::span<const uint8_t> image)
{
    return read_g64_header(image).ok;
}

std::vector<uint8_t> g64_decode_d64(std::span<const uint8_t> image)
{
    const g64_header h = read_g64_header(image);
    if (!h.ok)
    {
        return {};
    }

    std::vector<uint8_t> d64(k_d64_35_bytes, 0);
    std::vector<uint8_t> have_crc(683u, 0);

    for (uint8_t i = 0; i < h.track_count; ++i)
    {
        const uint8_t track = table_track_number(h.track_count, i);
        if (track < 1u || track > k_d64_track_count)
        {
            continue;
        }
        const std::size_t off_slot =
            k_g64_prefix_bytes + static_cast<std::size_t>(i) * 4u;
        const uint32_t off = read_le32(image, off_slot);
        if (off == 0u)
        {
            continue;
        }
        if (!in_range(image, static_cast<std::size_t>(off), 2u))
        {
            continue;
        }
        const uint16_t actual = read_le16(image, static_cast<std::size_t>(off));
        if (actual == 0u || actual > h.max_track_bytes)
        {
            continue;
        }
        const std::size_t data_off = static_cast<std::size_t>(off) + 2u;
        if (!in_range(image, data_off, static_cast<std::size_t>(actual)))
        {
            continue;
        }
        decode_gcr_track(image.subspan(data_off, actual), d64, have_crc);
    }
    return d64;
}

cbm_disk parse_g64(std::span<const uint8_t> image)
{
    if (!is_g64_image(image))
    {
        return {};
    }

    std::vector<uint8_t> d64 = g64_decode_d64(image);
    cbm_disk disk{};
    disk.present = true;
    disk.media = cbm_media::g64;
    disk.media_name = cbm_media_name(cbm_media::g64);
    if (d64.size() == k_d64_35_bytes)
    {
        const cbm_disk fs = parse_cbmfs(d64, cbm_media::d64);
        if (fs.present)
        {
            disk.disk_name = fs.disk_name;
            disk.disk_id = fs.disk_id;
            disk.dos_version = fs.dos_version;
            disk.dos_type = fs.dos_type;
            disk.dir_track = fs.dir_track;
            disk.dir_sector = fs.dir_sector;
            disk.entries = fs.entries;
        }
        disk.decoded = std::move(d64);
    }
    return disk;
}

} /* namespace dumpfloppy */
