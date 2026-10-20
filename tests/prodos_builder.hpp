/**
 * @file prodos_builder.hpp
 * @brief Synthetic ProDOS volume builder for tests (no commercial data).
 *
 * Writes a ProDOS-order block image with a volume directory at block 2,
 * subdirectories, seedling/sapling/tree files (with optional sparse
 * holes), and extended (storage type 5) files with a resource fork.
 */
#ifndef DUMPFLOPPY_TESTS_PRODOS_BUILDER_HPP
#define DUMPFLOPPY_TESTS_PRODOS_BUILDER_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace prodos_test
{

/** @brief Deterministic test bytes (xorshift), seeded per file. */
inline std::vector<uint8_t> pattern(std::size_t n, uint32_t seed)
{
    std::vector<uint8_t> v(n);
    uint32_t x = seed * 2654435761u + 1u;
    for (std::size_t i = 0; i < n; ++i)
    {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        v[i] = static_cast<uint8_t>(x);
    }
    return v;
}

/** @brief Fork location produced by @ref volume::put_fork. */
struct fork_ref
{
    uint8_t storage = 0;
    uint16_t key = 0;
    uint16_t blocks = 0;
    uint32_t eof = 0;
};

class volume
{
public:
    explicit volume(uint16_t total_blocks, std::string name, uint16_t case_bits = 0)
        : img_(static_cast<std::size_t>(total_blocks) * 512u, 0),
          total_(total_blocks)
    {
        /* Blocks 0-1 boot, 2-5 volume dir, 6 bitmap (enough for tests). */
        next_ = 7;
        root_ = make_dir(2, 4, name, 0x0Fu, 0, 0, 0, case_bits);
    }

    /** @brief Index of the volume directory. */
    [[nodiscard]] int root() const { return root_; }

    /** @brief Allocate one block (zeroed). */
    uint16_t alloc()
    {
        return next_++;
    }

    /**
     * @brief Store @p data as seedling/sapling/tree.
     *
     * @param hole_every When non-zero, every data block whose index is a
     *        multiple of this and that is all zero is left sparse.
     */
    fork_ref put_fork(const std::vector<uint8_t>& data, bool sparse_zero_blocks = false)
    {
        fork_ref r{};
        r.eof = static_cast<uint32_t>(data.size());
        const std::size_t nblk = std::max<std::size_t>(1u, (data.size() + 511u) / 512u);
        auto put_data = [&](std::size_t i) -> uint16_t
        {
            const std::size_t off = i * 512u;
            const std::size_t n = std::min<std::size_t>(512u, data.size() > off ? data.size() - off : 0u);
            bool zero = true;
            for (std::size_t k = 0; k < n; ++k)
            {
                zero = zero && data[off + k] == 0u;
            }
            if (sparse_zero_blocks && zero && i != 0u)
            {
                return 0;
            }
            const uint16_t b = alloc();
            ++r.blocks;
            if (n != 0u)
            {
                std::memcpy(blk(b), data.data() + off, n);
            }
            return b;
        };
        auto put_index = [&](std::size_t first) -> uint16_t
        {
            const uint16_t ib = alloc();
            ++r.blocks;
            for (std::size_t i = 0; i < 256u && first + i < nblk; ++i)
            {
                const uint16_t b = put_data(first + i);
                blk(ib)[i] = static_cast<uint8_t>(b);
                blk(ib)[i + 256u] = static_cast<uint8_t>(b >> 8);
            }
            return ib;
        };
        if (nblk == 1u)
        {
            r.storage = 1;
            r.key = put_data(0);
        }
        else if (nblk <= 256u)
        {
            r.storage = 2;
            r.key = put_index(0);
        }
        else
        {
            r.storage = 3;
            r.key = alloc();
            ++r.blocks;
            for (std::size_t m = 0; m * 256u < nblk; ++m)
            {
                const uint16_t ib = put_index(m * 256u);
                blk(r.key)[m] = static_cast<uint8_t>(ib);
                blk(r.key)[m + 256u] = static_cast<uint8_t>(ib >> 8);
            }
        }
        return r;
    }

    /** @brief Add a plain file to directory @p dir. */
    void add_file(int dir, const std::string& name, uint8_t type, uint16_t aux,
                  const std::vector<uint8_t>& data, bool sparse = false,
                  uint16_t case_bits = 0, uint8_t access = 0xC3)
    {
        const fork_ref f = put_fork(data, sparse);
        add_entry(dir, f.storage, name, type, f.key, f.blocks, f.eof, aux, case_bits, access);
    }

    /** @brief Add an extended (storage type 5) file with two forks. */
    void add_extended(int dir, const std::string& name, uint8_t type, uint16_t aux,
                      const std::vector<uint8_t>& data_fork,
                      const std::vector<uint8_t>& rsrc_fork)
    {
        const uint16_t kb = alloc();
        const fork_ref d = put_fork(data_fork);
        const fork_ref r = put_fork(rsrc_fork);
        uint8_t* p = blk(kb);
        auto mini = [&](std::size_t o, const fork_ref& f)
        {
            p[o] = f.storage;
            p[o + 1u] = static_cast<uint8_t>(f.key);
            p[o + 2u] = static_cast<uint8_t>(f.key >> 8);
            p[o + 3u] = static_cast<uint8_t>(f.blocks);
            p[o + 4u] = static_cast<uint8_t>(f.blocks >> 8);
            p[o + 5u] = static_cast<uint8_t>(f.eof);
            p[o + 6u] = static_cast<uint8_t>(f.eof >> 8);
            p[o + 7u] = static_cast<uint8_t>(f.eof >> 16);
        };
        mini(0, d);
        mini(0x100, r);
        add_entry(dir, 5, name, type, kb, static_cast<uint16_t>(1u + d.blocks + r.blocks),
                  512u, aux, 0, 0xE3);
    }

    /** @brief Add a subdirectory of @p nblocks blocks; returns its index. */
    int add_subdir(int parent, const std::string& name, uint16_t nblocks = 1)
    {
        const uint16_t first = next_;
        next_ = static_cast<uint16_t>(next_ + nblocks);
        const int idx = make_dir(first, nblocks, name, 0x0Eu, 0, 0, 0, 0);
        add_entry(parent, 0x0D, name, 0x0F, first, nblocks,
                  static_cast<uint32_t>(nblocks) * 512u, 0, 0, 0xC3);
        const dir_info& pd = dirs_[static_cast<std::size_t>(parent)];
        const uint16_t pblk = static_cast<uint16_t>(pd.first + pd.count / 13u);
        uint8_t* h = blk(first) + 4u;
        h[0x23] = static_cast<uint8_t>(pblk);
        h[0x24] = static_cast<uint8_t>(pblk >> 8);
        h[0x25] = static_cast<uint8_t>(pd.count % 13u + 1u);
        h[0x26] = 0x27;
        return idx;
    }

    /** @brief Mark the last entry of @p dir deleted (storage nibble 0). */
    void delete_last(int dir)
    {
        dir_info& d = dirs_[static_cast<std::size_t>(dir)];
        uint8_t* e = entry_ptr(d, d.count);
        e[0] = static_cast<uint8_t>(e[0] & 0x0Fu);
        dir_hdr(d)[0x21] = static_cast<uint8_t>(d.count - 1u);
    }

    /** @brief Pointer to raw block @p b (for corruption tests). */
    uint8_t* blk(uint16_t b) { return img_.data() + static_cast<std::size_t>(b) * 512u; }

    /** @brief Key block of directory @p dir. */
    [[nodiscard]] uint16_t dir_key(int dir) const
    {
        return dirs_[static_cast<std::size_t>(dir)].first;
    }

    /** @brief Image with the volume bitmap (block 6) written: 1 = free. */
    [[nodiscard]] std::vector<uint8_t> image() const
    {
        std::vector<uint8_t> out = img_;
        uint8_t* bm = out.data() + 6u * 512u;
        for (unsigned b = 0; b < total_ && b < 4096u; ++b)
        {
            if (b >= next_)
            {
                bm[b / 8u] = static_cast<uint8_t>(bm[b / 8u] | (0x80u >> (b % 8u)));
            }
        }
        return out;
    }

private:
    struct dir_info
    {
        uint16_t first = 0;
        uint16_t nblocks = 0;
        unsigned count = 0; /* active entries, header excluded */
    };

    uint8_t* entry_ptr(const dir_info& d, unsigned slot)
    {
        /* slot 0 is the header in the first block; 13 entries per block. */
        const unsigned b = slot / 13u;
        const unsigned i = slot % 13u;
        return blk(static_cast<uint16_t>(d.first + b)) + 4u + i * 0x27u;
    }

    uint8_t* dir_hdr(const dir_info& d) { return blk(d.first) + 4u; }

    int make_dir(uint16_t first, uint16_t nblocks, const std::string& name, uint8_t st,
                 uint16_t, uint16_t, uint16_t, uint16_t case_bits)
    {
        dir_info d{};
        d.first = first;
        d.nblocks = nblocks;
        for (uint16_t i = 0; i < nblocks; ++i)
        {
            uint8_t* p = blk(static_cast<uint16_t>(first + i));
            const uint16_t prev = i == 0u ? 0u : static_cast<uint16_t>(first + i - 1u);
            const uint16_t next = i + 1u == nblocks ? 0u : static_cast<uint16_t>(first + i + 1u);
            p[0] = static_cast<uint8_t>(prev);
            p[1] = static_cast<uint8_t>(prev >> 8);
            p[2] = static_cast<uint8_t>(next);
            p[3] = static_cast<uint8_t>(next >> 8);
        }
        uint8_t* h = blk(first) + 4u;
        h[0] = static_cast<uint8_t>((st << 4) | name.size());
        std::memcpy(h + 1, name.data(), name.size());
        if (st == 0x0Fu)
        {
            h[0x16] = static_cast<uint8_t>(case_bits);
            h[0x17] = static_cast<uint8_t>(case_bits >> 8);
            h[0x23] = 6; /* bitmap pointer */
            h[0x25] = static_cast<uint8_t>(total_);
            h[0x26] = static_cast<uint8_t>(total_ >> 8);
        }
        else
        {
            h[0x10] = 0x75;
        }
        h[0x1E] = 0xC3;
        h[0x1F] = 0x27;
        h[0x20] = 0x0D;
        dirs_.push_back(d);
        return static_cast<int>(dirs_.size() - 1u);
    }

    void add_entry(int dir, uint8_t storage, const std::string& name, uint8_t type,
                   uint16_t key, uint16_t blocks, uint32_t eof, uint16_t aux,
                   uint16_t case_bits, uint8_t access)
    {
        dir_info& d = dirs_[static_cast<std::size_t>(dir)];
        ++d.count;
        uint8_t* e = entry_ptr(d, d.count);
        e[0] = static_cast<uint8_t>((storage << 4) | name.size());
        std::memcpy(e + 1, name.data(), name.size());
        e[0x10] = type;
        e[0x11] = static_cast<uint8_t>(key);
        e[0x12] = static_cast<uint8_t>(key >> 8);
        e[0x13] = static_cast<uint8_t>(blocks);
        e[0x14] = static_cast<uint8_t>(blocks >> 8);
        e[0x15] = static_cast<uint8_t>(eof);
        e[0x16] = static_cast<uint8_t>(eof >> 8);
        e[0x17] = static_cast<uint8_t>(eof >> 16);
        e[0x1C] = static_cast<uint8_t>(case_bits);
        e[0x1D] = static_cast<uint8_t>(case_bits >> 8);
        e[0x1E] = access;
        e[0x1F] = static_cast<uint8_t>(aux);
        e[0x20] = static_cast<uint8_t>(aux >> 8);
        e[0x25] = static_cast<uint8_t>(d.first);
        e[0x26] = static_cast<uint8_t>(d.first >> 8);
        dir_hdr(d)[0x21] = static_cast<uint8_t>(d.count);
    }

    std::vector<uint8_t> img_;
    uint16_t total_ = 0;
    uint16_t next_ = 0;
    int root_ = 0;
    std::vector<dir_info> dirs_{};
};

/** @brief Inverse of dumpfloppy::apple_dos_order_to_prodos (ProDOS → DOS order). */
inline std::vector<uint8_t> to_dos_order(const std::vector<uint8_t>& po)
{
    static constexpr uint8_t k_map[16] = {0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15};
    std::vector<uint8_t> dos(po.size(), 0);
    for (std::size_t base = 0; base + 4096u <= po.size(); base += 4096u)
    {
        for (unsigned s = 0; s < 16u; ++s)
        {
            std::memcpy(dos.data() + base + static_cast<std::size_t>(k_map[s]) * 256u,
                        po.data() + base + static_cast<std::size_t>(s) * 256u, 256u);
        }
    }
    return dos;
}

/** @brief Wrap a payload in a 64-byte 2IMG header (format 0 DOS, 1 ProDOS). */
inline std::vector<uint8_t> wrap_2img(const std::vector<uint8_t>& payload, uint32_t format)
{
    std::vector<uint8_t> out(64u + payload.size(), 0);
    auto le32 = [&](std::size_t o, uint32_t v)
    {
        for (int i = 0; i < 4; ++i)
        {
            out[o + static_cast<std::size_t>(i)] = static_cast<uint8_t>(v >> (8 * i));
        }
    };
    std::memcpy(out.data(), "2IMG", 4);
    std::memcpy(out.data() + 4, "TEST", 4);
    out[8] = 64;
    out[10] = 1;
    le32(0x0C, format);
    le32(0x14, format == 1u ? static_cast<uint32_t>(payload.size() / 512u) : 0u);
    le32(0x18, 64u);
    le32(0x1C, static_cast<uint32_t>(payload.size()));
    std::copy(payload.begin(), payload.end(), out.begin() + 64);
    return out;
}

} /* namespace prodos_test */

#endif /* DUMPFLOPPY_TESTS_PRODOS_BUILDER_HPP */
