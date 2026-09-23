/**
 * @file g64.cpp
 * @brief GCR-1541 / GCR-1571 container parse and GCR track → sector decode.
 *
 * Layout and sector GCR follow Peter Schepers `G64.TXT` (signature,
 * half-track table, SYNC ≥10 ones, header ID $08, data ID $07). G71 uses
 * the same GCR with magic `GCR-1571` and a 1571 logical T/S (1–70).
 */
#include "dumpfloppy/g64.hpp"
#include "dumpfloppy/g71.hpp"
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

struct gcr_disk_header
{
    bool ok = false;
    bool is_g71 = false;
    uint8_t track_count = 0;
    uint16_t max_track_bytes = 0;
};

[[nodiscard]] gcr_disk_header read_gcr_disk_header(std::span<const uint8_t> image)
{
    gcr_disk_header h{};
    if (image.size() < k_g64_prefix_bytes)
    {
        return h;
    }
    const bool is_g64 = std::memcmp(image.data(), k_g64_magic, 8) == 0;
    const bool is_g71 = std::memcmp(image.data(), k_g71_magic, 8) == 0;
    if (!is_g64 && !is_g71)
    {
        return h;
    }
    if (image[8] != k_g64_version)
    {
        return h;
    }
    const uint8_t n = image[9];
    const uint8_t max_n = is_g71 ? k_g71_max_tracks : k_g64_max_tracks;
    if (n == 0u || n > max_n)
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
    h.is_g71 = is_g71;
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

void store_sector(std::vector<uint8_t>& image, std::vector<uint8_t>& have_crc,
                  cbm_media media, uint8_t track, uint8_t sector,
                  const uint8_t* data, bool crc_ok)
{
    if (!cbm_ts_valid(media, track, sector))
    {
        return;
    }
    const std::size_t off = cbm_offset(media, track, sector);
    if (off == static_cast<std::size_t>(-1) ||
        off + static_cast<std::size_t>(k_d64_sector_bytes) > image.size())
    {
        return;
    }
    const std::size_t idx = off / static_cast<std::size_t>(k_d64_sector_bytes);
    if (idx >= have_crc.size() || have_crc[idx] != 0u)
    {
        return;
    }
    std::memcpy(image.data() + off, data, k_d64_sector_bytes);
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

void decode_gcr_track(std::span<const uint8_t> gcr, std::vector<uint8_t>& image,
                      std::vector<uint8_t>& have_crc, cbm_media media)
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
                store_sector(image, have_crc, media, hdr_track, hdr_sector, blk + 1,
                             crc_ok);
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

/**
 * G64: skip half-tracks when n>42, and skip table tracks above 35.
 * G71: skip half-tracks only when n>84 (VICE 168-slot layout). An 84-slot
 * GCR-1571 stores whole tracks on both sides.
 */
[[nodiscard]] bool skip_gcr_slot(const gcr_disk_header& h, uint8_t index) noexcept
{
    if (h.is_g71)
    {
        return h.track_count > 84u && (index % 2u) != 0u;
    }
    const uint8_t track = table_track_number(h.track_count, index);
    return track < 1u || track > k_d64_track_count;
}

[[nodiscard]] std::vector<uint8_t>
decode_gcr_container(std::span<const uint8_t> image, const gcr_disk_header& h)
{
    const cbm_media store = h.is_g71 ? cbm_media::d71 : cbm_media::d64;
    const std::size_t out_size = h.is_g71 ? k_d71_bytes : k_d64_35_bytes;
    std::vector<uint8_t> out(out_size, 0);
    std::vector<uint8_t> have_crc(
        out_size / static_cast<std::size_t>(k_d64_sector_bytes), 0);

    for (uint8_t i = 0; i < h.track_count; ++i)
    {
        if (skip_gcr_slot(h, i))
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
        decode_gcr_track(image.subspan(data_off, actual), out, have_crc, store);
    }
    return out;
}

void fill_cbmfs_from_decoded(cbm_disk& disk, std::vector<uint8_t>&& decoded,
                             cbm_media fs_media)
{
    if (decoded.empty())
    {
        return;
    }
    const cbm_disk fs = parse_cbmfs(decoded, fs_media);
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
    disk.decoded = std::move(decoded);
}

} /* namespace */

bool is_g64_image(std::span<const uint8_t> image)
{
    const gcr_disk_header h = read_gcr_disk_header(image);
    return h.ok && !h.is_g71;
}

bool is_g71_image(std::span<const uint8_t> image)
{
    const gcr_disk_header h = read_gcr_disk_header(image);
    return h.ok && h.is_g71;
}

std::vector<uint8_t> g64_decode_d64(std::span<const uint8_t> image)
{
    const gcr_disk_header h = read_gcr_disk_header(image);
    if (!h.ok || h.is_g71)
    {
        return {};
    }
    return decode_gcr_container(image, h);
}

std::vector<uint8_t> g71_decode_d71(std::span<const uint8_t> image)
{
    const gcr_disk_header h = read_gcr_disk_header(image);
    if (!h.ok || !h.is_g71)
    {
        return {};
    }
    return decode_gcr_container(image, h);
}

cbm_disk parse_g64(std::span<const uint8_t> image)
{
    if (!is_g64_image(image))
    {
        return {};
    }

    cbm_disk disk{};
    disk.present = true;
    disk.media = cbm_media::g64;
    disk.media_name = cbm_media_name(cbm_media::g64);
    fill_cbmfs_from_decoded(disk, g64_decode_d64(image), cbm_media::d64);
    return disk;
}

cbm_disk parse_g71(std::span<const uint8_t> image)
{
    if (!is_g71_image(image))
    {
        return {};
    }

    cbm_disk disk{};
    disk.present = true;
    disk.media = cbm_media::g71;
    disk.media_name = cbm_media_name(cbm_media::g71);
    fill_cbmfs_from_decoded(disk, g71_decode_d71(image), cbm_media::d71);
    return disk;
}

} /* namespace dumpfloppy */
