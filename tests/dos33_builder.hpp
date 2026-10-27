/**
 * @file dos33_builder.hpp
 * @brief Synthetic Apple DOS 3.3 140K images for catalog tests.
 *
 * Builds a 35×16 DOS-order volume: VTOC at T17/S0, a 15-sector catalog
 * chain T17/S15 → T17/S1, and files whose T/S lists and data are
 * allocated from track 18 upward. Every byte is generated here; no real
 * disk content is used.
 */
#ifndef DUMPFLOPPY_TESTS_DOS33_BUILDER_HPP
#define DUMPFLOPPY_TESTS_DOS33_BUILDER_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace dos33_test
{

inline constexpr std::size_t k_sector = 256u;
inline constexpr unsigned k_tracks = 35u;
inline constexpr unsigned k_spt = 16u;

/** @brief DOS 3.3 file type bytes (bit 7 = locked). */
enum : uint8_t
{
    t_text = 0x00,
    t_integer = 0x01,
    t_applesoft = 0x02,
    t_binary = 0x04,
    t_s = 0x08,
    t_relocatable = 0x10,
    t_new_a = 0x20,
    t_new_b = 0x40,
    t_locked = 0x80
};

/** @brief Where one catalog entry was written. */
struct slot
{
    std::size_t off = 0; /**< Byte offset of the 35-byte entry. */
    uint8_t ts_track = 0;
    uint8_t ts_sector = 0;
};

class builder
{
public:
    builder() : img_(k_tracks * k_spt * k_sector, 0)
    {
        const std::size_t vtoc = at(17, 0);
        img_[vtoc + 0x01u] = 17;
        img_[vtoc + 0x02u] = 15;
        img_[vtoc + 0x03u] = 3;
        img_[vtoc + 0x06u] = 254;
        img_[vtoc + 0x27u] = 122;
        img_[vtoc + 0x34u] = static_cast<uint8_t>(k_tracks);
        img_[vtoc + 0x35u] = static_cast<uint8_t>(k_spt);
        img_[vtoc + 0x36u] = 0x00;
        img_[vtoc + 0x37u] = 0x01;
        for (unsigned s = 15; s >= 1u; --s)
        {
            const std::size_t c = at(17, s);
            img_[c + 1u] = static_cast<uint8_t>(s > 1u ? 17u : 0u);
            img_[c + 2u] = s > 1u ? static_cast<uint8_t>(s - 1u) : 0u;
        }
    }

    [[nodiscard]] static std::size_t at(unsigned t, unsigned s)
    {
        return (static_cast<std::size_t>(t) * k_spt + s) * k_sector;
    }

    /** @brief Next free sector from T18/S0 upward (catalog track skipped). */
    std::pair<uint8_t, uint8_t> alloc()
    {
        if (next_t_ >= k_tracks)
        {
            throw std::runtime_error("synthetic DOS 3.3 volume full");
        }
        const auto r = std::make_pair(static_cast<uint8_t>(next_t_), static_cast<uint8_t>(next_s_));
        if (++next_s_ == k_spt)
        {
            next_s_ = 0;
            ++next_t_;
        }
        return r;
    }

    /**
     * @brief Add a file whose file sector i is @p sectors[i]; an empty
     *        element is a hole (zero T/S pair).
     */
    slot add_sparse(const std::string& name, uint8_t type,
                    const std::vector<std::vector<uint8_t>>& sectors)
    {
        std::vector<std::pair<uint8_t, uint8_t>> lists;
        const std::size_t n_lists = sectors.empty() ? 1u : (sectors.size() + 121u) / 122u;
        for (std::size_t i = 0; i < n_lists; ++i)
        {
            lists.push_back(alloc());
        }
        unsigned used = static_cast<unsigned>(n_lists);
        for (std::size_t i = 0; i < sectors.size(); ++i)
        {
            if (sectors[i].empty())
            {
                continue;
            }
            const auto [dt, ds] = alloc();
            ++used;
            std::memcpy(img_.data() + at(dt, ds), sectors[i].data(),
                        std::min<std::size_t>(sectors[i].size(), k_sector));
            const auto [lt, ls] = lists[i / 122u];
            const std::size_t p = at(lt, ls) + 0x0Cu + (i % 122u) * 2u;
            img_[p] = dt;
            img_[p + 1u] = ds;
        }
        for (std::size_t i = 0; i < lists.size(); ++i)
        {
            const std::size_t l = at(lists[i].first, lists[i].second);
            if (i + 1u < lists.size())
            {
                img_[l + 1u] = lists[i + 1u].first;
                img_[l + 2u] = lists[i + 1u].second;
            }
            const unsigned off = static_cast<unsigned>(i * 122u);
            img_[l + 5u] = static_cast<uint8_t>(off);
            img_[l + 6u] = static_cast<uint8_t>(off >> 8);
        }
        return add_entry(name, type, lists[0].first, lists[0].second, used);
    }

    /** @brief Add a file holding @p body, split into whole sectors. */
    slot add(const std::string& name, uint8_t type, const std::vector<uint8_t>& body)
    {
        std::vector<std::vector<uint8_t>> secs;
        for (std::size_t o = 0; o < body.size(); o += k_sector)
        {
            std::vector<uint8_t> s(k_sector, 0);
            std::memcpy(s.data(), body.data() + o, std::min(k_sector, body.size() - o));
            secs.push_back(std::move(s));
        }
        return add_sparse(name, type, secs);
    }

    /** @brief DOS DELETE: T/S track to the last name byte, then $FF. */
    void remove(const slot& sl)
    {
        img_[sl.off + 0x20u] = img_[sl.off];
        img_[sl.off] = 0xFF;
    }

    [[nodiscard]] std::vector<uint8_t>& bytes() { return img_; }

private:
    slot add_entry(const std::string& name, uint8_t type, uint8_t t, uint8_t s, unsigned secs)
    {
        const unsigned idx = entries_++;
        const unsigned cat_s = 15u - idx / 7u;
        if (cat_s < 1u)
        {
            throw std::runtime_error("synthetic DOS 3.3 catalog full");
        }
        const std::size_t e = at(17, cat_s) + 0x0Bu + (idx % 7u) * 35u;
        img_[e] = t;
        img_[e + 1u] = s;
        img_[e + 2u] = type;
        for (std::size_t i = 0; i < 30u; ++i)
        {
            const char c = i < name.size() ? name[i] : ' ';
            img_[e + 3u + i] = static_cast<uint8_t>(static_cast<unsigned char>(c) | 0x80u);
        }
        img_[e + 33u] = static_cast<uint8_t>(secs);
        img_[e + 34u] = static_cast<uint8_t>(secs >> 8);
        return slot{e, t, s};
    }

    std::vector<uint8_t> img_;
    unsigned next_t_ = 18;
    unsigned next_s_ = 0;
    unsigned entries_ = 0;
};

/** @brief File body with a little-endian header word prefix. */
inline std::vector<uint8_t> with_header(std::vector<uint16_t> words, std::size_t body_len,
                                        uint8_t fill)
{
    std::vector<uint8_t> out;
    for (const uint16_t w : words)
    {
        out.push_back(static_cast<uint8_t>(w));
        out.push_back(static_cast<uint8_t>(w >> 8));
    }
    for (std::size_t i = 0; i < body_len; ++i)
    {
        out.push_back(static_cast<uint8_t>(fill + i));
    }
    return out;
}

} /* namespace dos33_test */

#endif /* DUMPFLOPPY_TESTS_DOS33_BUILDER_HPP */
