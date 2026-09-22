/**
 * @file image_builder.hpp
 * @brief Tiny in-memory FAT12 images for Catch2 tests.
 */
#ifndef DUMPFLOPPY_TEST_IMAGE_BUILDER_HPP
#define DUMPFLOPPY_TEST_IMAGE_BUILDER_HPP

#include "dumpfloppy/fat12_codec.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace dumpfloppy_test
{

inline constexpr uint16_t k_bps = 512;
inline constexpr uint8_t k_spc = 1;
inline constexpr uint16_t k_reserved = 1;
inline constexpr uint8_t k_fats = 2;
inline constexpr uint16_t k_root_ent = 16;
inline constexpr uint16_t k_total_sec = 64;
inline constexpr uint8_t k_media = 0xF8;
inline constexpr uint16_t k_spf = 1;
inline constexpr uint16_t k_spt = 8;
inline constexpr uint16_t k_heads = 1;

inline void poke_le16(uint8_t* p, uint16_t v)
{
    p[0] = static_cast<uint8_t>(v & 0xFFu);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
}

inline void poke_le32(uint8_t* p, uint32_t v)
{
    p[0] = static_cast<uint8_t>(v & 0xFFu);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
    p[2] = static_cast<uint8_t>((v >> 16) & 0xFFu);
    p[3] = static_cast<uint8_t>((v >> 24) & 0xFFu);
}

inline void put_name11(uint8_t* e, const char* name11)
{
    std::memset(e, ' ', 11);
    const size_t n = std::strlen(name11);
    std::memcpy(e, name11, n > 11u ? 11u : n);
}

/**
 * @brief Build a 32 KiB FAT12 floppy with serial, labels, live + deleted files.
 *
 * @param[in] serial EBPB volume serial (0x1234ABCD → `1234-ABCD`).
 * @param[in] with_ebpb If false, omit 0x29 (DOS 3.3 style).
 */
inline std::vector<uint8_t> make_fat12_sample(uint32_t serial = 0x1234ABCDu,
                                              bool with_ebpb = true)
{
    std::vector<uint8_t> img(static_cast<size_t>(k_total_sec) * k_bps, 0);

    uint8_t* b = img.data();
    b[0] = 0xEB;
    b[1] = 0x3C;
    b[2] = 0x90;
    std::memcpy(b + 3, "DUMPFLPY", 8);
    poke_le16(b + 11, k_bps);
    b[13] = k_spc;
    poke_le16(b + 14, k_reserved);
    b[16] = k_fats;
    poke_le16(b + 17, k_root_ent);
    poke_le16(b + 19, k_total_sec);
    b[21] = k_media;
    poke_le16(b + 22, k_spf);
    poke_le16(b + 24, k_spt);
    poke_le16(b + 26, k_heads);
    poke_le32(b + 28, 0);
    poke_le32(b + 32, 0);

    if (with_ebpb)
    {
        b[0x24] = 0x00;
        b[0x25] = 0x00;
        b[0x26] = 0x29;
        poke_le32(b + 0x27, serial);
        std::memcpy(b + 0x2B, "TESTVOL    ", 11);
        std::memcpy(b + 0x36, "FAT12   ", 8);
    }

    const char* msg = "Non-System disk or disk error";
    std::memcpy(b + 0x100, msg, std::strlen(msg));
    std::memcpy(b + 0x1C0, "IO      SYS", 11);
    std::memcpy(b + 0x1CB, "MSDOS   SYS", 11);
    b[510] = 0x55;
    b[511] = 0xAA;

    uint8_t* fat0 = img.data() + static_cast<size_t>(k_reserved) * k_bps;
    const size_t fat_len = static_cast<size_t>(k_spf) * k_bps;
    fat12_entry_set(fat0, fat_len, 0, static_cast<uint16_t>(0xF00u | k_media));
    fat12_entry_set(fat0, fat_len, 1, 0xFFF);
    fat12_entry_set(fat0, fat_len, 2, 0xFFF); /* HELLO.TXT */
    fat12_entry_set(fat0, fat_len, 3, 0xFFF); /* deleted GONE.TXT still chained */

    uint8_t* fat1 = fat0 + fat_len;
    std::memcpy(fat1, fat0, fat_len);

    uint8_t* root = fat1 + fat_len;
    /* 0: volume label */
    put_name11(root, "TESTVOL    ");
    root[11] = 0x08;
    /* 1: HELLO.TXT */
    uint8_t* hello = root + 32;
    put_name11(hello, "HELLO   TXT");
    hello[11] = 0x20;
    poke_le16(hello + 26, 2);
    poke_le32(hello + 28, 14);
    poke_le16(hello + 24, (15u << 9) | (9u << 5) | 22u); /* 1995-09-22 */
    poke_le16(hello + 22, (12u << 11) | (0u << 5) | 0u);
    /* 2: deleted GONE.TXT */
    uint8_t* gone = root + 64;
    put_name11(gone, "GONE    TXT");
    gone[0] = 0xE5;
    gone[11] = 0x20;
    poke_le16(gone + 26, 3);
    poke_le32(gone + 28, 4);

    const size_t data = static_cast<size_t>(k_reserved + k_fats * k_spf + 1u) * k_bps;
    std::memcpy(img.data() + data, "Hello, floppy\n", 14);
    std::memcpy(img.data() + data + k_bps, "BYE\n", 4);

    return img;
}

/** @brief Custom booter: JMP + 55 AA, no FAT BPB. */
inline std::vector<uint8_t> make_booter_sample()
{
    std::vector<uint8_t> img(512, 0);
    img[0] = 0xEB;
    img[1] = 0x10;
    img[2] = 0x90;
    std::memcpy(img.data() + 0x20, "LOADING GAME", 12);
    img[510] = 0x55;
    img[511] = 0xAA;
    return img;
}

} /* namespace dumpfloppy_test */

#endif /* DUMPFLOPPY_TEST_IMAGE_BUILDER_HPP */
