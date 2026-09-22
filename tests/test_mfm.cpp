/**
 * @file test_mfm.cpp
 * @brief HxC CHS assembly (BPB / modal SPT) and HLS vs that geometry.
 */
#include "dumpfloppy/ibm_mfm.hpp"
#include "dumpfloppy/image.hpp"
#include "image_builder.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <string>
#include <vector>

namespace
{

uint16_t crc16_ibm(const uint8_t* data, size_t n)
{
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < n; ++i)
    {
        crc = static_cast<uint16_t>(crc ^ static_cast<uint16_t>(data[i] << 8));
        for (int b = 0; b < 8; ++b)
        {
            if ((crc & 0x8000u) != 0u)
            {
                crc = static_cast<uint16_t>((crc << 1) ^ 0x1021u);
            }
            else
            {
                crc = static_cast<uint16_t>(crc << 1);
            }
        }
    }
    return crc;
}

void mfm_byte(std::vector<uint8_t>& bits, uint8_t val, uint8_t& prev)
{
    for (int b = 7; b >= 0; --b)
    {
        const uint8_t data = static_cast<uint8_t>((val >> static_cast<unsigned>(b)) & 1u);
        const uint8_t clock = (prev == 0u && data == 0u) ? 1u : 0u;
        bits.push_back(clock);
        bits.push_back(data);
        prev = data;
    }
}

void mfm_sync_a1(std::vector<uint8_t>& bits, uint8_t& prev)
{
    static constexpr uint8_t k_sync[16] = {0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1};
    bits.insert(bits.end(), k_sync, k_sync + 16);
    prev = 1;
}

void append_idam(std::vector<uint8_t>& bits, uint8_t cyl, uint8_t head, uint8_t sec,
                 uint8_t size_code, uint8_t& prev)
{
    const uint8_t rec[8] = {0xA1, 0xA1, 0xA1, 0xFE, cyl, head, sec, size_code};
    const uint16_t crc = crc16_ibm(rec, sizeof(rec));
    mfm_sync_a1(bits, prev);
    mfm_sync_a1(bits, prev);
    mfm_sync_a1(bits, prev);
    mfm_byte(bits, 0xFE, prev);
    mfm_byte(bits, cyl, prev);
    mfm_byte(bits, head, prev);
    mfm_byte(bits, sec, prev);
    mfm_byte(bits, size_code, prev);
    mfm_byte(bits, static_cast<uint8_t>(crc >> 8), prev);
    mfm_byte(bits, static_cast<uint8_t>(crc & 0xFFu), prev);
}

void append_dam(std::vector<uint8_t>& bits, uint8_t mark, std::span<const uint8_t> payload,
                uint8_t& prev)
{
    std::vector<uint8_t> rec(4u + payload.size());
    rec[0] = 0xA1;
    rec[1] = 0xA1;
    rec[2] = 0xA1;
    rec[3] = mark;
    if (!payload.empty())
    {
        std::memcpy(rec.data() + 4, payload.data(), payload.size());
    }
    const uint16_t crc = crc16_ibm(rec.data(), rec.size());
    mfm_sync_a1(bits, prev);
    mfm_sync_a1(bits, prev);
    mfm_sync_a1(bits, prev);
    mfm_byte(bits, mark, prev);
    for (uint8_t b : payload)
    {
        mfm_byte(bits, b, prev);
    }
    mfm_byte(bits, static_cast<uint8_t>(crc >> 8), prev);
    mfm_byte(bits, static_cast<uint8_t>(crc & 0xFFu), prev);
}

std::vector<uint8_t> pack_bits(const std::vector<uint8_t>& bits)
{
    std::vector<uint8_t> out((bits.size() + 7u) / 8u, 0);
    for (size_t i = 0; i < bits.size(); ++i)
    {
        if (bits[i] != 0u)
        {
            const unsigned shift = static_cast<unsigned>(7u - (i % 8u));
            out[i / 8u] =
                static_cast<uint8_t>(out[i / 8u] | static_cast<uint8_t>(1u << shift));
        }
    }
    return out;
}

void append_sector(std::vector<uint8_t>& bits, uint8_t cyl, uint8_t head, uint8_t sec,
                   std::span<const uint8_t> payload512, uint8_t& prev)
{
    for (int i = 0; i < 8; ++i)
    {
        mfm_byte(bits, 0x00, prev);
    }
    append_idam(bits, cyl, head, sec, 2, prev);
    for (int i = 0; i < 8; ++i)
    {
        mfm_byte(bits, 0x00, prev);
    }
    append_dam(bits, 0xFB, payload512, prev);
}

std::vector<uint8_t> wrap_hxc(uint32_t cyls, uint8_t sides,
                              const std::vector<std::vector<uint8_t>>& track_blobs)
{
    const uint32_t nside = (sides == 0u) ? 1u : static_cast<uint32_t>(sides);
    const uint32_t n = cyls * nside;
    REQUIRE(track_blobs.size() == n);
    const uint32_t list_off = 19;
    const uint32_t data0 = list_off + n * 11u;
    std::vector<uint8_t> f(data0, 0);
    std::memcpy(f.data(), "HXCMFM", 6);
    f[7] = static_cast<uint8_t>(cyls & 0xFFu);
    f[8] = static_cast<uint8_t>((cyls >> 8) & 0xFFu);
    f[9] = sides;
    f[10] = 44;
    f[11] = 1;
    f[12] = 250;
    f[13] = 0;
    f[15] = static_cast<uint8_t>(list_off & 0xFFu);
    f[16] = static_cast<uint8_t>((list_off >> 8) & 0xFFu);
    f[17] = static_cast<uint8_t>((list_off >> 16) & 0xFFu);
    f[18] = static_cast<uint8_t>((list_off >> 24) & 0xFFu);
    uint32_t toff = data0;
    for (uint32_t t = 0; t < n; ++t)
    {
        uint8_t* e = f.data() + list_off + t * 11u;
        const uint32_t cyl = t / nside;
        const uint32_t hd = t % nside;
        e[0] = static_cast<uint8_t>(cyl & 0xFFu);
        e[1] = static_cast<uint8_t>((cyl >> 8) & 0xFFu);
        e[2] = static_cast<uint8_t>(hd);
        const uint32_t sz = static_cast<uint32_t>(track_blobs[t].size());
        e[3] = static_cast<uint8_t>(sz & 0xFFu);
        e[4] = static_cast<uint8_t>((sz >> 8) & 0xFFu);
        e[5] = static_cast<uint8_t>((sz >> 16) & 0xFFu);
        e[6] = static_cast<uint8_t>((sz >> 24) & 0xFFu);
        e[7] = static_cast<uint8_t>(toff & 0xFFu);
        e[8] = static_cast<uint8_t>((toff >> 8) & 0xFFu);
        e[9] = static_cast<uint8_t>((toff >> 16) & 0xFFu);
        e[10] = static_cast<uint8_t>((toff >> 24) & 0xFFu);
        toff += sz;
    }
    for (const std::vector<uint8_t>& blob : track_blobs)
    {
        f.insert(f.end(), blob.begin(), blob.end());
    }
    return f;
}

dumpfloppy::ibm_sector make_sec(uint8_t cyl, uint8_t head, uint8_t sec, uint8_t fill)
{
    dumpfloppy::ibm_sector s{};
    s.cyl = cyl;
    s.head = head;
    s.sector = sec;
    s.size_code = 2;
    s.bytes = 512;
    s.idam_crc_ok = true;
    s.dam_crc_ok = true;
    s.has_dam = true;
    s.data.assign(512, fill);
    s.data[0] = cyl;
    s.data[1] = head;
    s.data[2] = sec;
    return s;
}

std::vector<uint8_t> make_boot(uint16_t spt, uint16_t heads, uint16_t total_sec)
{
    std::vector<uint8_t> b(512, 0);
    dumpfloppy_test::write_min_fat12_boot(b.data());
    dumpfloppy_test::poke_le16(b.data() + 24, spt);
    dumpfloppy_test::poke_le16(b.data() + 26, heads);
    dumpfloppy_test::poke_le16(b.data() + 19, total_sec);
    return b;
}

bool hls_mentions_sector(const dumpfloppy::flux_disk& d, unsigned sector)
{
    char needle[32] = {};
    std::snprintf(needle, sizeof(needle), ":S%u (", sector);
    for (const std::string& p : d.protection)
    {
        if (p.find("HLS-style") != std::string::npos &&
            p.find(needle) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

bool any_hls(const dumpfloppy::flux_disk& d)
{
    for (const std::string& p : d.protection)
    {
        if (p.find("HLS-style") != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

} /* namespace */

TEST_CASE("9-spt tracks plus extra ID 11 keep chs_spt 9", "[mfm][chs]")
{
    dumpfloppy::flux_disk d{};
    d.boot = make_boot(9, 1, 36);
    for (uint8_t c = 0; c < 2; ++c)
    {
        for (uint8_t sec = 1; sec <= 9; ++sec)
        {
            dumpfloppy::ibm_sector s = make_sec(c, 0, sec, static_cast<uint8_t>(0xA0u + sec));
            if (c == 0u && sec == 1u)
            {
                s.data = d.boot;
            }
            d.sectors.push_back(std::move(s));
        }
    }
    dumpfloppy::ibm_sector extra = make_sec(0, 0, 11, 0xB1);
    extra.data.assign(512, 0xB1);
    extra.data[2] = 11;
    d.sectors.push_back(std::move(extra));

    dumpfloppy::finish_ibm_flux(d);

    REQUIRE(d.chs_spt == 9);
    REQUIRE(d.chs_heads == 1);
    REQUIRE(d.chs_cyls == 2);
    REQUIRE(d.assembled_chs.size() == 2u * 1u * 9u * 512u);
    REQUIRE(d.assembled_chs.size() != 2u * 1u * 11u * 512u);
    REQUIRE(hls_mentions_sector(d, 11));
    for (unsigned sec = 1; sec <= 9; ++sec)
    {
        REQUIRE_FALSE(hls_mentions_sector(d, sec));
    }
    /* If SPT had inflated to 11, C0:H0:S11 would land at LBA 10. */
    REQUIRE(d.assembled_chs.size() > 10u * 512u);
    REQUIRE(d.assembled_chs[10u * 512u] != 0xB1);
    bool saw_b1_sector = false;
    for (size_t lba = 0; lba + 512u <= d.assembled_chs.size(); lba += 512u)
    {
        if (d.assembled_chs[lba + 2u] == 11u && d.assembled_chs[lba] == 0xB1)
        {
            saw_b1_sector = true;
        }
    }
    REQUIRE_FALSE(saw_b1_sector);
}

TEST_CASE("modal 9-spt ignores a one-off ID 11 without a BPB", "[mfm][chs]")
{
    dumpfloppy::flux_disk d{};
    for (uint8_t c = 0; c < 2; ++c)
    {
        for (uint8_t sec = 1; sec <= 9; ++sec)
        {
            d.sectors.push_back(make_sec(c, 0, sec, 0xCC));
        }
    }
    d.sectors.push_back(make_sec(0, 0, 11, 0xB1));
    dumpfloppy::finish_ibm_flux(d);
    REQUIRE(d.chs_spt == 9);
    REQUIRE(d.assembled_chs.size() == 2u * 9u * 512u);
    REQUIRE(hls_mentions_sector(d, 11));
}

TEST_CASE("18-spt sectors 10-18 are not HLS", "[mfm][hls]")
{
    dumpfloppy::flux_disk d{};
    for (uint8_t sec = 1; sec <= 18; ++sec)
    {
        d.sectors.push_back(make_sec(0, 0, sec, static_cast<uint8_t>(0x10u + sec)));
    }
    dumpfloppy::finish_ibm_flux(d);
    REQUIRE(d.chs_spt == 18);
    REQUIRE(d.chs_heads == 1);
    REQUIRE(d.assembled_chs.size() == 18u * 512u);
    REQUIRE_FALSE(any_hls(d));
    for (unsigned sec = 10; sec <= 18; ++sec)
    {
        REQUIRE_FALSE(hls_mentions_sector(d, sec));
        const size_t off = static_cast<size_t>(sec - 1u) * 512u;
        REQUIRE(d.assembled_chs[off + 2u] == static_cast<uint8_t>(sec));
    }
}

TEST_CASE("a single extra head IDAM does not raise chs_heads", "[mfm][chs]")
{
    dumpfloppy::flux_disk d{};
    for (uint8_t h = 0; h < 2; ++h)
    {
        for (uint8_t sec = 1; sec <= 9; ++sec)
        {
            d.sectors.push_back(make_sec(0, h, sec, 0x33));
        }
    }
    d.sectors.push_back(make_sec(0, 2, 1, 0xEE));
    dumpfloppy::finish_ibm_flux(d);
    REQUIRE(d.chs_spt == 9);
    REQUIRE(d.chs_heads == 2);
    REQUIRE(d.assembled_chs.size() == 2u * 9u * 512u);
    REQUIRE(hls_mentions_sector(d, 1)); /* C0:H2:S1 is nonstandard vs heads=2 */
}

TEST_CASE("BPB SPT 21 (DMF) is used and sector 21 is in the image", "[mfm][chs]")
{
    dumpfloppy::flux_disk d{};
    d.boot = make_boot(21, 1, 21);
    for (uint8_t sec = 1; sec <= 21; ++sec)
    {
        dumpfloppy::ibm_sector s = make_sec(0, 0, sec, 0x21);
        if (sec == 1u)
        {
            s.data = d.boot;
        }
        d.sectors.push_back(std::move(s));
    }
    dumpfloppy::finish_ibm_flux(d);
    REQUIRE(d.chs_spt == 21);
    REQUIRE(d.assembled_chs.size() == 21u * 512u);
    REQUIRE_FALSE(hls_mentions_sector(d, 21));
    REQUIRE(d.assembled_chs[20u * 512u + 2u] == 21);
}

TEST_CASE("assembled_chs is capped to BPB total_sectors", "[mfm][chs]")
{
    dumpfloppy::flux_disk d{};
    d.boot = make_boot(9, 1, 4);
    for (uint8_t sec = 1; sec <= 9; ++sec)
    {
        dumpfloppy::ibm_sector s = make_sec(0, 0, sec, 0x44);
        if (sec == 1u)
        {
            s.data = d.boot;
        }
        d.sectors.push_back(std::move(s));
    }
    dumpfloppy::finish_ibm_flux(d);
    REQUIRE(d.chs_spt == 9);
    REQUIRE(d.assembled_chs.size() == 4u * 512u);
    REQUIRE(d.assembled_chs.size() <= dumpfloppy::k_max_image_bytes);
}

TEST_CASE("decode_hxc_mfm 9-spt blob plus ID 11 does not inflate SPT", "[mfm][hxc]")
{
    std::vector<uint8_t> bits;
    uint8_t prev = 1;
    std::vector<uint8_t> payload(512, 0);
    for (uint8_t sec = 1; sec <= 9; ++sec)
    {
        payload.assign(512, static_cast<uint8_t>(0xA0u + sec));
        payload[2] = sec;
        if (sec == 1u)
        {
            payload = make_boot(9, 1, 18);
        }
        append_sector(bits, 0, 0, sec, payload, prev);
    }
    for (uint8_t sec = 1; sec <= 9; ++sec)
    {
        payload.assign(512, static_cast<uint8_t>(0xB0u + sec));
        payload[0] = 1;
        payload[2] = sec;
        append_sector(bits, 1, 0, sec, payload, prev);
    }
    payload.assign(512, 0xB1);
    payload[2] = 11;
    append_sector(bits, 0, 0, 11, payload, prev);

    const std::vector<uint8_t> track = pack_bits(bits);
    const std::vector<uint8_t> blob = wrap_hxc(1, 1, {track});
    const dumpfloppy::flux_disk d = dumpfloppy::decode_hxc_mfm(blob);
    REQUIRE(d.present);
    REQUIRE(d.format_name == "HXC MFM");
    REQUIRE(d.chs_spt == 9);
    REQUIRE(d.chs_heads == 1);
    REQUIRE(d.assembled_chs.size() == 2u * 9u * 512u);
    REQUIRE(hls_mentions_sector(d, 11));
    for (unsigned sec = 1; sec <= 9; ++sec)
    {
        REQUIRE_FALSE(hls_mentions_sector(d, sec));
    }
    REQUIRE(d.assembled_chs.size() > 10u * 512u);
    REQUIRE(d.assembled_chs[10u * 512u] != 0xB1);
}

TEST_CASE("decode_hxc_mfm 18-spt blob does not flag S10-S18 as HLS", "[mfm][hxc][hls]")
{
    std::vector<uint8_t> bits;
    uint8_t prev = 1;
    std::vector<uint8_t> payload(512, 0);
    for (uint8_t sec = 1; sec <= 18; ++sec)
    {
        payload.assign(512, static_cast<uint8_t>(0x30u + sec));
        payload[2] = sec;
        append_sector(bits, 0, 0, sec, payload, prev);
    }
    const std::vector<uint8_t> blob = wrap_hxc(1, 1, {pack_bits(bits)});
    const dumpfloppy::flux_disk d = dumpfloppy::decode_hxc_mfm(blob);
    REQUIRE(d.present);
    REQUIRE(d.chs_spt == 18);
    REQUIRE_FALSE(any_hls(d));
    for (unsigned sec = 10; sec <= 18; ++sec)
    {
        REQUIRE_FALSE(hls_mentions_sector(d, sec));
    }
}
