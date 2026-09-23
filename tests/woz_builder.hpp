/**
 * @file woz_builder.hpp
 * @brief Encode a DOS-order 140K Apple volume into a WOZ2 for Catch2 tests.
 */
#ifndef DUMPFLOPPY_TEST_WOZ_BUILDER_HPP
#define DUMPFLOPPY_TEST_WOZ_BUILDER_HPP

#include "dumpfloppy/apple.hpp"
#include "dumpfloppy/apple_gcr_codec.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace dumpfloppy_test
{

struct woz_bit_writer
{
    std::vector<uint8_t> bytes{};
    uint8_t acc = 0;
    unsigned filled = 0;
    std::size_t nbits = 0;

    void put_bit(unsigned bit)
    {
        acc = static_cast<uint8_t>(static_cast<uint8_t>(acc << 1) | (bit & 1u));
        ++filled;
        ++nbits;
        if (filled == 8u)
        {
            bytes.push_back(acc);
            acc = 0;
            filled = 0;
        }
    }

    void put_byte(uint8_t value)
    {
        for (unsigned i = 0; i < 8u; ++i)
        {
            const unsigned shift = 7u - i;
            put_bit(static_cast<unsigned>((value >> shift) & 1u));
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
    img[off] = static_cast<uint8_t>(v);
    img[off + 1u] = static_cast<uint8_t>(v >> 8);
}

inline void poke_le32(std::vector<uint8_t>& img, std::size_t off, uint32_t v)
{
    img[off] = static_cast<uint8_t>(v);
    img[off + 1u] = static_cast<uint8_t>(v >> 8);
    img[off + 2u] = static_cast<uint8_t>(v >> 16);
    img[off + 3u] = static_cast<uint8_t>(v >> 24);
}

inline void put_4n4(woz_bit_writer& w, uint8_t value)
{
    uint8_t odd = 0;
    uint8_t even = 0;
    apple_gcr_encode_4n4(value, &odd, &even);
    w.put_byte(odd);
    w.put_byte(even);
}

inline woz_bit_writer encode_dos_track(const std::vector<uint8_t>& dos, uint8_t track)
{
    woz_bit_writer w{};
    constexpr uint8_t vol = 254;
    for (uint8_t s = 0; s < 16u; ++s)
    {
        const std::size_t off = (static_cast<std::size_t>(track) * 16u + s) * 256u;
        for (unsigned i = 0; i < 20u; ++i)
        {
            w.put_byte(0xFFu);
        }
        w.put_byte(0xD5);
        w.put_byte(0xAA);
        w.put_byte(0x96);
        put_4n4(w, vol);
        put_4n4(w, track);
        put_4n4(w, s);
        put_4n4(w, static_cast<uint8_t>(vol ^ track ^ s));
        w.put_byte(0xDE);
        w.put_byte(0xAA);
        w.put_byte(0xEB);
        for (unsigned i = 0; i < 5u; ++i)
        {
            w.put_byte(0xFFu);
        }
        w.put_byte(0xD5);
        w.put_byte(0xAA);
        w.put_byte(0xAD);
        uint8_t nib[APPLE_GCR_NIBBLE_BYTES] = {};
        apple_gcr_encode_sector(dos.data() + off, nib);
        for (uint8_t b : nib)
        {
            w.put_byte(b);
        }
        w.put_byte(0xDE);
        w.put_byte(0xAA);
        w.put_byte(0xEB);
        for (unsigned i = 0; i < 5u; ++i)
        {
            w.put_byte(0xFFu);
        }
    }
    w.pad_to_byte();
    return w;
}

/**
 * @brief Wrap a 35-track DOS-order 140K image in a WOZ2 with 6-and-2 tracks.
 *
 * @param[in] dos 143360-byte DOS 3.3 (or DOS-order ProDOS) volume.
 */
inline std::vector<uint8_t> dos_to_woz2(const std::vector<uint8_t>& dos)
{
    constexpr std::size_t k_need = dumpfloppy::k_apple_dos33_140k;
    if (dos.size() < k_need)
    {
        return {};
    }

    std::vector<woz_bit_writer> tracks;
    tracks.reserve(35);
    std::size_t max_blocks = 0;
    for (uint8_t t = 0; t < 35u; ++t)
    {
        tracks.push_back(encode_dos_track(dos, t));
        const std::size_t blocks = (tracks.back().bytes.size() + 511u) / 512u;
        if (blocks > max_blocks)
        {
            max_blocks = blocks;
        }
    }
    if (max_blocks == 0u)
    {
        max_blocks = 1;
    }

    constexpr std::size_t k_bits0 = 1536;
    std::vector<uint8_t> img(k_bits0, 0);
    std::memcpy(img.data(), "WOZ2", 4);
    img[4] = 0xFFu;
    img[5] = 0x0Au;
    img[6] = 0x0Du;
    img[7] = 0x0Au;

    std::memcpy(img.data() + 12, "INFO", 4);
    poke_le32(img, 16, 60u);
    img[20] = 2;
    img[21] = 1;
    img[23] = 1;
    const char* cr = "dumpfloppy test";
    std::memcpy(img.data() + 25, cr, 15);
    for (std::size_t i = 25 + 15; i < 25 + 32; ++i)
    {
        img[i] = ' ';
    }
    img[57] = 1;
    img[58] = 1;
    img[59] = 32;
    poke_le16(img, 64, static_cast<uint16_t>(max_blocks));

    std::memcpy(img.data() + 80, "TMAP", 4);
    poke_le32(img, 84, 160u);
    std::memset(img.data() + 88, 0xFF, 160);
    for (uint8_t t = 0; t < 35u; ++t)
    {
        img[88 + static_cast<std::size_t>(t) * 4u] = t;
    }

    std::memcpy(img.data() + 248, "TRKS", 4);
    uint16_t block = 3;
    for (uint8_t t = 0; t < 35u; ++t)
    {
        const std::size_t e = 256u + static_cast<std::size_t>(t) * 8u;
        const std::size_t nbytes = tracks[t].bytes.size();
        const uint16_t nblk = static_cast<uint16_t>((nbytes + 511u) / 512u);
        poke_le16(img, e, block);
        poke_le16(img, e + 2u, nblk);
        poke_le32(img, e + 4u, static_cast<uint32_t>(tracks[t].nbits));
        const std::size_t bit_off = static_cast<std::size_t>(block) * 512u;
        if (img.size() < bit_off + static_cast<std::size_t>(nblk) * 512u)
        {
            img.resize(bit_off + static_cast<std::size_t>(nblk) * 512u, 0);
        }
        if (nbytes > 0u)
        {
            std::memcpy(img.data() + bit_off, tracks[t].bytes.data(), nbytes);
        }
        block = static_cast<uint16_t>(block + nblk);
    }
    poke_le32(img, 252, static_cast<uint32_t>(img.size() - 256u));
    return img;
}

} /* namespace dumpfloppy_test */

#endif /* DUMPFLOPPY_TEST_WOZ_BUILDER_HPP */
