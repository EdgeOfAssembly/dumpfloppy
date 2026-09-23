/**
 * @file adf_builder.hpp
 * @brief Tiny in-memory OFS/FFS ADF images for Catch2 tests.
 */
#ifndef DUMPFLOPPY_TEST_ADF_BUILDER_HPP
#define DUMPFLOPPY_TEST_ADF_BUILDER_HPP

#include "dumpfloppy/amiga.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <vector>

namespace dumpfloppy_test
{

inline constexpr uint32_t k_auto_hash_slot = 0xFFFFFFFFu;

inline void poke_be32(uint8_t* p, uint32_t v) noexcept
{
    p[0] = static_cast<uint8_t>(v >> 24);
    p[1] = static_cast<uint8_t>(v >> 16);
    p[2] = static_cast<uint8_t>(v >> 8);
    p[3] = static_cast<uint8_t>(v);
}

[[nodiscard]] inline uint32_t peek_be32(const uint8_t* p) noexcept
{
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}

inline void checksum_block(uint8_t* blk) noexcept
{
    poke_be32(blk + 20, 0u);
    uint32_t sum = 0;
    for (unsigned i = 0; i < dumpfloppy::k_adf_sector_bytes; i += 4u)
    {
        sum += peek_be32(blk + i);
    }
    poke_be32(blk + 20, static_cast<uint32_t>(0u - sum));
}

inline void set_bcpl(uint8_t* blk, std::string_view name) noexcept
{
    unsigned n = static_cast<unsigned>(name.size());
    if (n > dumpfloppy::k_amiga_name_max)
    {
        n = dumpfloppy::k_amiga_name_max;
    }
    blk[0x1B0] = static_cast<uint8_t>(n);
    if (n != 0u)
    {
        std::memcpy(blk + 0x1B1, name.data(), static_cast<std::size_t>(n));
    }
}

/**
 * @brief Classic AmigaDOS hash (length, ×13, ASCII upper, `& 0x7ff`, `% ht`).
 *
 * @param[in] name    File/dir name as stored.
 * @param[in] ht_size Hash table size (72 for 512-byte blocks).
 */
[[nodiscard]] inline uint32_t amiga_hash_name(std::string_view name,
                                              uint32_t ht_size = dumpfloppy::k_adf_ht_size)
{
    uint32_t hash = static_cast<uint32_t>(name.size());
    for (const char ch : name)
    {
        auto c = static_cast<unsigned char>(ch);
        hash *= 13u;
        if (c >= static_cast<unsigned char>('a') && c <= static_cast<unsigned char>('z'))
        {
            c = static_cast<unsigned char>(c - 32u);
        }
        hash += static_cast<uint32_t>(c);
        hash &= 0x7FFu;
    }
    if (ht_size == 0u)
    {
        return 0u;
    }
    return hash % ht_size;
}

/** @brief Mutable ADF plus a bump allocator (skips boot 0–1 and the root). */
struct adf_builder
{
    std::vector<uint8_t> bytes{};
    uint32_t next_block = 2;
    uint32_t root_block = 880;
    uint32_t sector_count = dumpfloppy::k_adf_dd_sectors;
    bool ffs = false;

    [[nodiscard]] uint8_t* ptr(uint32_t block)
    {
        return bytes.data() +
               static_cast<std::size_t>(block) *
                   static_cast<std::size_t>(dumpfloppy::k_adf_sector_bytes);
    }

    [[nodiscard]] uint32_t alloc()
    {
        while (next_block < sector_count)
        {
            const uint32_t b = next_block;
            ++next_block;
            if (b != root_block && b >= 2u)
            {
                return b;
            }
        }
        return 0u;
    }

    void checksum(uint32_t block)
    {
        checksum_block(ptr(block));
    }

    void link_hash(uint32_t dir, uint32_t entry, std::string_view name, uint32_t force_slot)
    {
        uint32_t slot = force_slot;
        if (slot >= dumpfloppy::k_adf_ht_size)
        {
            slot = amiga_hash_name(name, dumpfloppy::k_adf_ht_size);
        }
        uint8_t* d = ptr(dir);
        uint8_t* e = ptr(entry);
        const uint32_t old =
            peek_be32(d + 24u + static_cast<std::size_t>(slot) * 4u);
        poke_be32(e + 496, old);
        poke_be32(d + 24u + static_cast<std::size_t>(slot) * 4u, entry);
    }

    uint32_t make_dir(uint32_t parent, std::string_view name)
    {
        const uint32_t blk = alloc();
        if (blk == 0u)
        {
            return 0u;
        }
        uint8_t* p = ptr(blk);
        std::memset(p, 0, dumpfloppy::k_adf_sector_bytes);
        poke_be32(p + 0, 2u);
        poke_be32(p + 4, blk);
        poke_be32(p + 12, dumpfloppy::k_adf_ht_size);
        set_bcpl(p, name);
        poke_be32(p + 500, parent);
        poke_be32(p + 508, static_cast<uint32_t>(dumpfloppy::k_amiga_st_userdir));
        link_hash(parent, blk, name, k_auto_hash_slot);
        checksum(blk);
        checksum(parent);
        return blk;
    }

    uint32_t make_file(uint32_t parent, std::string_view name, std::span<const uint8_t> data,
                       uint32_t force_slot = k_auto_hash_slot)
    {
        const uint32_t header = alloc();
        if (header == 0u)
        {
            return 0u;
        }

        const std::size_t chunk = ffs ? static_cast<std::size_t>(dumpfloppy::k_adf_sector_bytes)
                                      : static_cast<std::size_t>(dumpfloppy::k_ofs_data_payload);
        const std::size_t nblocks =
            data.empty() ? 0u : (data.size() + chunk - 1u) / chunk;
        std::vector<uint32_t> keys;
        keys.reserve(nblocks);

        std::size_t pos = 0;
        uint32_t seq = 1;
        for (std::size_t bi = 0; bi < nblocks; ++bi)
        {
            const uint32_t db = alloc();
            if (db == 0u)
            {
                return 0u;
            }
            keys.push_back(db);
            uint8_t* blk = ptr(db);
            std::memset(blk, 0, dumpfloppy::k_adf_sector_bytes);
            const std::size_t take = std::min(chunk, data.size() - pos);
            if (ffs)
            {
                std::memcpy(blk, data.data() + pos, take);
            }
            else
            {
                poke_be32(blk + 0, 8u);
                poke_be32(blk + 4, header);
                poke_be32(blk + 8, seq);
                poke_be32(blk + 12, static_cast<uint32_t>(take));
                std::memcpy(blk + 24, data.data() + pos, take);
            }
            pos += take;
            ++seq;
        }

        if (!ffs)
        {
            for (std::size_t i = 0; i < keys.size(); ++i)
            {
                const uint32_t next = (i + 1u < keys.size()) ? keys[i + 1u] : 0u;
                poke_be32(ptr(keys[i]) + 16, next);
                checksum(keys[i]);
            }
        }

        auto fill_pointers = [&](uint8_t* dest, std::size_t begin, std::size_t count) {
            for (std::size_t i = 0; i < count; ++i)
            {
                const std::size_t off =
                    24u + (static_cast<std::size_t>(dumpfloppy::k_adf_ht_size) - 1u - i) * 4u;
                poke_be32(dest + off, keys[begin + i]);
            }
        };

        std::vector<uint32_t> meta;
        meta.push_back(header);

        uint8_t* h = ptr(header);
        std::memset(h, 0, dumpfloppy::k_adf_sector_bytes);
        poke_be32(h + 0, 2u);
        poke_be32(h + 4, header);
        const std::size_t in_hdr = std::min(nblocks, static_cast<std::size_t>(72));
        poke_be32(h + 8, static_cast<uint32_t>(in_hdr));
        if (!keys.empty())
        {
            poke_be32(h + 16, keys[0]);
        }
        poke_be32(h + 0x144, static_cast<uint32_t>(data.size()));
        set_bcpl(h, name);
        poke_be32(h + 500, parent);
        poke_be32(h + 508, static_cast<uint32_t>(dumpfloppy::k_amiga_st_file));
        fill_pointers(h, 0, in_hdr);

        std::size_t filled = in_hdr;
        uint32_t prev = header;
        while (filled < nblocks)
        {
            const uint32_t ext = alloc();
            if (ext == 0u)
            {
                return 0u;
            }
            poke_be32(ptr(prev) + 504, ext);
            uint8_t* e = ptr(ext);
            std::memset(e, 0, dumpfloppy::k_adf_sector_bytes);
            poke_be32(e + 0, 16u);
            poke_be32(e + 4, header);
            const std::size_t in_ext =
                std::min(nblocks - filled, static_cast<std::size_t>(72));
            poke_be32(e + 8, static_cast<uint32_t>(in_ext));
            fill_pointers(e, filled, in_ext);
            poke_be32(e + 500, header);
            poke_be32(e + 508, static_cast<uint32_t>(dumpfloppy::k_amiga_st_file));
            meta.push_back(ext);
            filled += in_ext;
            prev = ext;
        }

        link_hash(parent, header, name, force_slot);
        for (const uint32_t m : meta)
        {
            checksum(m);
        }
        checksum(parent);
        return header;
    }
};

[[nodiscard]] inline adf_builder make_adf(uint8_t dos_type, std::string_view volume,
                                          bool hd = false)
{
    adf_builder b{};
    b.ffs = dumpfloppy::amiga_dos_is_ffs(dos_type);
    b.sector_count =
        hd ? dumpfloppy::k_adf_hd_sectors : dumpfloppy::k_adf_dd_sectors;
    b.root_block = dumpfloppy::adf_root_block(b.sector_count);
    b.next_block = 2;
    const std::size_t nbytes =
        static_cast<std::size_t>(b.sector_count) *
        static_cast<std::size_t>(dumpfloppy::k_adf_sector_bytes);
    b.bytes.assign(nbytes, 0);
    b.bytes[0] = static_cast<uint8_t>('D');
    b.bytes[1] = static_cast<uint8_t>('O');
    b.bytes[2] = static_cast<uint8_t>('S');
    b.bytes[3] = dos_type;
    uint8_t* root = b.ptr(b.root_block);
    poke_be32(root + 0, 2u);
    poke_be32(root + 12, dumpfloppy::k_adf_ht_size);
    set_bcpl(root, volume);
    poke_be32(root + 508, static_cast<uint32_t>(dumpfloppy::k_amiga_st_root));
    b.checksum(b.root_block);
    return b;
}

/** @brief 500-byte OFS payload (two data blocks: 488 + 12). */
[[nodiscard]] inline std::vector<uint8_t> sample_ofs_payload()
{
    std::vector<uint8_t> p(500);
    for (std::size_t i = 0; i < p.size(); ++i)
    {
        p[i] = static_cast<uint8_t>(i & 0xFFu);
    }
    const char tag[] = "OFS-PAYLOAD";
    std::memcpy(p.data(), tag, sizeof(tag) - 1u);
    return p;
}

/** @brief 600-byte FFS payload (512 + 88); distinctive bytes at 0 and 512. */
[[nodiscard]] inline std::vector<uint8_t> sample_ffs_payload()
{
    std::vector<uint8_t> p(600);
    for (std::size_t i = 0; i < p.size(); ++i)
    {
        p[i] = static_cast<uint8_t>(0xA5u ^ static_cast<uint8_t>(i & 0xFFu));
    }
    const char tag[] = "FFS-RAW-BLOCK0";
    std::memcpy(p.data(), tag, sizeof(tag) - 1u);
    p[512] = 0xBBu;
    p[513] = 0xCCu;
    return p;
}

[[nodiscard]] inline std::vector<uint8_t> sample_inner_payload()
{
    const char tag[] = "INNER-DATA";
    return std::vector<uint8_t>(tag, tag + sizeof(tag) - 1u);
}

[[nodiscard]] inline std::vector<uint8_t> make_ofs_root_file_adf()
{
    adf_builder b = make_adf(0, "TESTADF", false);
    const std::vector<uint8_t> payload = sample_ofs_payload();
    b.make_file(b.root_block, "README", payload);
    return std::move(b.bytes);
}

[[nodiscard]] inline std::vector<uint8_t> make_ofs_subdir_adf()
{
    adf_builder b = make_adf(0, "TESTADF", false);
    const uint32_t sub = b.make_dir(b.root_block, "Sub");
    const std::vector<uint8_t> payload = sample_inner_payload();
    b.make_file(sub, "Inner", payload);
    return std::move(b.bytes);
}

[[nodiscard]] inline std::vector<uint8_t> make_ffs_root_file_adf()
{
    adf_builder b = make_adf(1, "FFSVOL", false);
    const std::vector<uint8_t> payload = sample_ffs_payload();
    b.make_file(b.root_block, "RAWFILE", payload);
    return std::move(b.bytes);
}

/** @brief FFS file with 73 data blocks so the header extension at 504 is used. */
[[nodiscard]] inline std::vector<uint8_t> make_ffs_extension_adf()
{
    adf_builder b = make_adf(1, "EXTVOL", false);
    const std::size_t n = 72u * 512u + 16u;
    std::vector<uint8_t> payload(n);
    for (std::size_t i = 0; i < payload.size(); ++i)
    {
        payload[i] = static_cast<uint8_t>((i * 131u + 7u) & 0xFFu);
    }
    payload[0] = static_cast<uint8_t>('E');
    payload[1] = static_cast<uint8_t>('X');
    payload[2] = static_cast<uint8_t>('T');
    payload[72u * 512u] = 0xEEu;
    b.make_file(b.root_block, "BIGFILE", payload);
    return std::move(b.bytes);
}

[[nodiscard]] inline std::vector<uint8_t> make_hd_ofs_adf()
{
    adf_builder b = make_adf(0, "HDROOT", true);
    const char tag[] = "HD-FILE";
    const std::vector<uint8_t> payload(tag, tag + sizeof(tag) - 1u);
    b.make_file(b.root_block, "HDFILE", payload);
    return std::move(b.bytes);
}

/** @brief Two root files forced into hash slot 0 (hash_chain at 496). */
[[nodiscard]] inline std::vector<uint8_t> make_ofs_hash_chain_adf()
{
    adf_builder b = make_adf(0, "CHAIN", false);
    const char a[] = "AAA";
    const char c[] = "CCC";
    const std::vector<uint8_t> pa(a, a + sizeof(a) - 1u);
    const std::vector<uint8_t> pc(c, c + sizeof(c) - 1u);
    b.make_file(b.root_block, "Alpha", pa, 0u);
    b.make_file(b.root_block, "Beta", pc, 0u);
    return std::move(b.bytes);
}

} /* namespace dumpfloppy_test */

#endif /* DUMPFLOPPY_TEST_ADF_BUILDER_HPP */
