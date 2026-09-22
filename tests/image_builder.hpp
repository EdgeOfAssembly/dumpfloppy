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
/** Inclusive last data cluster on the 64-sector geometry (clusters 2..61). */
inline constexpr uint32_t k_last_cluster = 61;

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

inline void write_min_fat12_boot(uint8_t* b)
{
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
    b[510] = 0x55;
    b[511] = 0xAA;
}

inline size_t fat12_fat_len()
{
    return static_cast<size_t>(k_spf) * k_bps;
}

inline uint8_t* fat12_fat0(std::vector<uint8_t>& img)
{
    return img.data() + static_cast<size_t>(k_reserved) * k_bps;
}

inline uint8_t* fat12_root(std::vector<uint8_t>& img)
{
    return fat12_fat0(img) + static_cast<size_t>(k_fats) * fat12_fat_len();
}

inline size_t fat12_data_off()
{
    return static_cast<size_t>(k_reserved + k_fats * k_spf + 1u) * k_bps;
}

inline void fat12_init_media(uint8_t* fat0, size_t fat_len)
{
    fat12_entry_set(fat0, fat_len, 0, static_cast<uint16_t>(0xF00u | k_media));
    fat12_entry_set(fat0, fat_len, 1, 0xFFF);
}

inline void fat12_chain(uint8_t* fat0, size_t fat_len, uint32_t first, uint32_t last)
{
    for (uint32_t c = first; c < last; ++c)
    {
        fat12_entry_set(fat0, fat_len, c, static_cast<uint16_t>(c + 1u));
    }
    fat12_entry_set(fat0, fat_len, last, 0xFFF);
}

inline void fat12_mirror_fat1(std::vector<uint8_t>& img)
{
    uint8_t* fat0 = fat12_fat0(img);
    const size_t n = fat12_fat_len();
    std::memcpy(fat0 + n, fat0, n);
}

inline void put_file_dirent(uint8_t* e, const char* name11, uint16_t first, uint32_t size,
                            bool deleted = false)
{
    put_name11(e, name11);
    e[11] = 0x20;
    poke_le16(e + 26, first);
    poke_le32(e + 28, size);
    if (deleted)
    {
        e[0] = 0xE5;
    }
}

/** @brief Distinctive 600-byte live TACTICS.PKG payload for reuse fixtures. */
inline std::vector<uint8_t> tactics_live_bytes()
{
    std::vector<uint8_t> p(600, static_cast<uint8_t>('T'));
    const char tag[] = "TACTICS-LIVE";
    std::memcpy(p.data(), tag, sizeof(tag) - 1u);
    return p;
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

/**
 * @brief Two live files in consecutive clusters so a grow must relocate FILEB.
 *
 * FILEA.TXT occupies clusters 2–3 (600 bytes). FILEB.TXT occupies cluster 4
 * (10 bytes). Growing FILEA to three clusters relocates FILEB.
 */
inline std::vector<uint8_t> make_fat12_packed()
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
    b[510] = 0x55;
    b[511] = 0xAA;

    uint8_t* fat0 = img.data() + static_cast<size_t>(k_reserved) * k_bps;
    const size_t fat_len = static_cast<size_t>(k_spf) * k_bps;
    fat12_entry_set(fat0, fat_len, 0, static_cast<uint16_t>(0xF00u | k_media));
    fat12_entry_set(fat0, fat_len, 1, 0xFFF);
    fat12_entry_set(fat0, fat_len, 2, 3);    /* FILEA.TXT */
    fat12_entry_set(fat0, fat_len, 3, 0xFFF);
    fat12_entry_set(fat0, fat_len, 4, 0xFFF); /* FILEB.TXT */

    uint8_t* fat1 = fat0 + fat_len;
    std::memcpy(fat1, fat0, fat_len);

    uint8_t* root = fat1 + fat_len;
    uint8_t* aent = root;
    put_name11(aent, "FILEA   TXT");
    aent[11] = 0x20;
    poke_le16(aent + 26, 2);
    poke_le32(aent + 28, 600);
    uint8_t* bent = root + 32;
    put_name11(bent, "FILEB   TXT");
    bent[11] = 0x20;
    poke_le16(bent + 26, 4);
    poke_le32(bent + 28, 10);

    const size_t data = static_cast<size_t>(k_reserved + k_fats * k_spf + 1u) * k_bps;
    std::vector<uint8_t> a(600, static_cast<uint8_t>('A'));
    std::memcpy(img.data() + data, a.data(), a.size());
    std::memcpy(img.data() + data + 2u * k_bps, "FILEB-DATA", 10);

    return img;
}

/**
 * @brief One file occupying every data cluster (grow must fail).
 */
inline std::vector<uint8_t> make_fat12_full()
{
    std::vector<uint8_t> img = make_fat12_sample();
    uint8_t* fat0 = img.data() + static_cast<size_t>(k_reserved) * k_bps;
    const size_t fat_len = static_cast<size_t>(k_spf) * k_bps;
    /* Data clusters 2 .. 61 (60 clusters). Chain HELLO through all of them. */
    constexpr uint32_t k_last = 61;
    for (uint32_t c = 2; c < k_last; ++c)
    {
        fat12_entry_set(fat0, fat_len, c, static_cast<uint16_t>(c + 1u));
    }
    fat12_entry_set(fat0, fat_len, k_last, 0xFFF);
    /* Cluster 3 was GONE.TXT; keep it in HELLO's chain and drop the deleted slot. */
    uint8_t* fat1 = fat0 + fat_len;
    std::memcpy(fat1, fat0, fat_len);
    uint8_t* root = fat1 + fat_len;
    uint8_t* hello = root + 32;
    poke_le16(hello + 26, 2);
    poke_le32(hello + 28, 60u * k_bps);
    std::memset(root + 64, 0, 32); /* erase deleted GONE.TXT */
    return img;
}

/**
 * @brief Live TACTICS.PKG + deleted dirent sharing `first_cluster` (Star Control).
 *
 * Clusters 2–3 hold the live 600-byte package. Deleted `?ACTICS.PKG` names
 * cluster 2 as well, so a naive FAT walk of the deleted slot follows the live
 * chain. HELLO.TXT occupies the last cluster (61). FILLER.BIN takes 4–60.
 * There are no free clusters: growing HELLO must go through reclaim and must
 * not `fat_set(0)` on TACTICS's clusters.
 */
inline std::vector<uint8_t> make_fat12_tactics_reuse()
{
    std::vector<uint8_t> img(static_cast<size_t>(k_total_sec) * k_bps, 0);
    write_min_fat12_boot(img.data());
    uint8_t* fat0 = fat12_fat0(img);
    const size_t fat_len = fat12_fat_len();
    fat12_init_media(fat0, fat_len);
    fat12_chain(fat0, fat_len, 2, 3);                 /* TACTICS.PKG */
    fat12_chain(fat0, fat_len, 4, 60);                /* FILLER.BIN */
    fat12_entry_set(fat0, fat_len, 61, 0xFFF);        /* HELLO.TXT */
    fat12_mirror_fat1(img);

    uint8_t* root = fat12_root(img);
    const std::vector<uint8_t> tactics = tactics_live_bytes();
    put_file_dirent(root, "TACTICS PKG", 2, static_cast<uint32_t>(tactics.size()));
    put_file_dirent(root + 32, "TACTICS PKG", 2, 400, true);
    put_file_dirent(root + 64, "HELLO   TXT", 61, 14);
    put_file_dirent(root + 96, "FILLER  BIN", 4, 57u * k_bps);

    const size_t data = fat12_data_off();
    std::memcpy(img.data() + data, tactics.data(), tactics.size());
    std::memcpy(img.data() + data + static_cast<size_t>(61u - 2u) * k_bps, "Hello, floppy\n",
                14);
    return img;
}

/**
 * @brief Live TACTICS.PKG + deleted dirent, same `first_cluster` and same size.
 *
 * Clusters 2–3 hold the live 600-byte package. Deleted `?ACTICS.PKG` names
 * cluster 2 and size 600, so a FAT walk of the deleted slot would hash and
 * extract the live payload. Occupancy-aware recovery must yield an empty
 * chain. Cluster 4 is HELLO.TXT; cluster 5 is deleted GONE.TXT (still
 * allocated) so orphan undelete still recovers the four-byte `BYE` payload.
 */
inline std::vector<uint8_t> make_fat12_tactics_same_cluster()
{
    std::vector<uint8_t> img(static_cast<size_t>(k_total_sec) * k_bps, 0);
    write_min_fat12_boot(img.data());
    uint8_t* fat0 = fat12_fat0(img);
    const size_t fat_len = fat12_fat_len();
    fat12_init_media(fat0, fat_len);
    fat12_chain(fat0, fat_len, 2, 3);          /* live TACTICS.PKG */
    fat12_entry_set(fat0, fat_len, 4, 0xFFF);  /* HELLO.TXT */
    fat12_entry_set(fat0, fat_len, 5, 0xFFF);  /* deleted GONE.TXT */
    fat12_mirror_fat1(img);

    uint8_t* root = fat12_root(img);
    const std::vector<uint8_t> tactics = tactics_live_bytes();
    const uint32_t tsz = static_cast<uint32_t>(tactics.size());
    put_file_dirent(root, "TACTICS PKG", 2, tsz);
    put_file_dirent(root + 32, "TACTICS PKG", 2, tsz, true);
    put_file_dirent(root + 64, "HELLO   TXT", 4, 14);
    put_file_dirent(root + 96, "GONE    TXT", 5, 4, true);

    const size_t data = fat12_data_off();
    std::memcpy(img.data() + data, tactics.data(), tactics.size());
    std::memcpy(img.data() + data + 2u * k_bps, "Hello, floppy\n", 14);
    std::memcpy(img.data() + data + 3u * k_bps, "BYE\n", 4);
    return img;
}

/**
 * @brief Deleted dirent starts on an orphan cluster; the next cluster is live.
 *
 * Cluster 2 holds leftover `OLD-HEAD` (FAT EOC, no live owner). Cluster 3 is
 * live NEW.BIN. Deleted OLD.BIN names first_cluster 2 and size 600, so a
 * contiguous undelete that ignored occupancy would swallow the live file.
 */
inline std::vector<uint8_t> make_fat12_deleted_before_live()
{
    std::vector<uint8_t> img(static_cast<size_t>(k_total_sec) * k_bps, 0);
    write_min_fat12_boot(img.data());
    uint8_t* fat0 = fat12_fat0(img);
    const size_t fat_len = fat12_fat_len();
    fat12_init_media(fat0, fat_len);
    fat12_entry_set(fat0, fat_len, 2, 0xFFF); /* orphan leftover */
    fat12_entry_set(fat0, fat_len, 3, 0xFFF); /* live NEW.BIN */
    fat12_mirror_fat1(img);

    uint8_t* root = fat12_root(img);
    put_file_dirent(root, "OLD     BIN", 2, 600, true);
    put_file_dirent(root + 32, "NEW     BIN", 3, 512);

    const size_t data = fat12_data_off();
    const char old_tag[] = "OLD-HEAD leftover that is not the live file";
    std::memcpy(img.data() + data, old_tag, sizeof(old_tag) - 1u);
    std::vector<uint8_t> live(512, static_cast<uint8_t>('N'));
    const char new_tag[] = "NEW-LIVE-FILE";
    std::memcpy(live.data(), new_tag, sizeof(new_tag) - 1u);
    std::memcpy(img.data() + data + k_bps, live.data(), live.size());
    return img;
}

/**
 * @brief Deleted GONE.TXT still owns cluster 3; the rest of the disk is full.
 *
 * Growing HELLO.TXT needs that orphan cluster. Reclaim must free it (it is
 * not live-owned) so the grow can succeed.
 */
inline std::vector<uint8_t> make_fat12_orphan_tight()
{
    std::vector<uint8_t> img = make_fat12_sample();
    uint8_t* fat0 = fat12_fat0(img);
    const size_t fat_len = fat12_fat_len();
    fat12_chain(fat0, fat_len, 4, k_last_cluster);
    fat12_mirror_fat1(img);
    uint8_t* root = fat12_root(img);
    put_file_dirent(root + 96, "FILLER  BIN", 4,
                    (k_last_cluster - 4u + 1u) * k_bps);
    return img;
}

/**
 * @brief FILEA (cluster 2) blocked by a 10-cluster FILEB with only 5 free.
 *
 * Growing FILEA wants cluster 3. Relocating FILEB needs 10 free clusters and
 * only 5 exist, so relocate must fail and the grow must abort.
 */
inline std::vector<uint8_t> make_fat12_relocate_tight()
{
    std::vector<uint8_t> img(static_cast<size_t>(k_total_sec) * k_bps, 0);
    write_min_fat12_boot(img.data());
    uint8_t* fat0 = fat12_fat0(img);
    const size_t fat_len = fat12_fat_len();
    fat12_init_media(fat0, fat_len);
    fat12_entry_set(fat0, fat_len, 2, 0xFFF);         /* FILEA.TXT */
    fat12_chain(fat0, fat_len, 3, 12);                /* FILEB.TXT */
    fat12_chain(fat0, fat_len, 13, 56);               /* FILLER.BIN */
    /* 57–61 left free (5 clusters). */
    fat12_mirror_fat1(img);

    uint8_t* root = fat12_root(img);
    put_file_dirent(root, "FILEA   TXT", 2, 10);
    put_file_dirent(root + 32, "FILEB   TXT", 3, 10);
    put_file_dirent(root + 64, "FILLER  BIN", 13, 44u * k_bps);

    const size_t data = fat12_data_off();
    std::memcpy(img.data() + data, "FILEA-DATA", 10);
    std::memcpy(img.data() + data + k_bps, "FILEB-DATA", 10);
    return img;
}

/**
 * @brief FILEA@2, FILEB@3 (1 cluster), deleted orphan at 61, FILLER 4–60.
 *
 * Growing FILEA wants cluster 3. Relocating FILEB needs one free cluster;
 * only the deleted orphan can provide it. Reclaim must run *before* relocate
 * or the grow aborts with no free dest. Would fail if reclaim ran only after
 * a failed neighbour move.
 */
inline std::vector<uint8_t> make_fat12_reclaim_then_relocate()
{
    std::vector<uint8_t> img(static_cast<size_t>(k_total_sec) * k_bps, 0);
    write_min_fat12_boot(img.data());
    uint8_t* fat0 = fat12_fat0(img);
    const size_t fat_len = fat12_fat_len();
    fat12_init_media(fat0, fat_len);
    fat12_entry_set(fat0, fat_len, 2, 0xFFF);  /* FILEA.TXT */
    fat12_entry_set(fat0, fat_len, 3, 0xFFF);  /* FILEB.TXT */
    fat12_chain(fat0, fat_len, 4, 60);         /* FILLER.BIN */
    fat12_entry_set(fat0, fat_len, 61, 0xFFF); /* deleted GONE.TXT */
    fat12_mirror_fat1(img);

    uint8_t* root = fat12_root(img);
    put_file_dirent(root, "FILEA   TXT", 2, 10);
    put_file_dirent(root + 32, "FILEB   TXT", 3, 10);
    put_file_dirent(root + 64, "FILLER  BIN", 4, 57u * k_bps);
    put_file_dirent(root + 96, "GONE    TXT", 61, 4, true);

    const size_t data = fat12_data_off();
    std::memcpy(img.data() + data, "FILEA-DATA", 10);
    std::memcpy(img.data() + data + k_bps, "FILEB-DATA", 10);
    std::memcpy(img.data() + data + static_cast<size_t>(61u - 2u) * k_bps, "BYE\n", 4);
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
