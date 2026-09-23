/**
 * @file amiga.cpp
 * @brief ADF OFS/FFS parser (boot, root hash walk, file data keys).
 */
#include "dumpfloppy/amiga.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace dumpfloppy
{
namespace
{

constexpr uint32_t k_off_type = 0u;
constexpr uint32_t k_off_high_seq = 8u;
constexpr uint32_t k_off_ht_size = 12u;
constexpr uint32_t k_off_data_size = 12u;
constexpr uint32_t k_off_table = 24u;
constexpr uint32_t k_off_byte_size = 0x144u;
constexpr uint32_t k_off_name = 0x1B0u;
constexpr uint32_t k_off_hash_chain = 496u;
constexpr uint32_t k_off_parent = 500u;
constexpr uint32_t k_off_extension = 504u;
constexpr uint32_t k_off_sec_type = 508u;

constexpr int32_t k_t_header = 2;
constexpr int32_t k_t_data = 8;
constexpr int32_t k_t_list = 16;

constexpr int k_max_visits = 4096;
constexpr unsigned k_name_chars = 30u;

[[nodiscard]] uint32_t read_be32(std::span<const uint8_t> data, std::size_t off) noexcept
{
    if (off + 4u > data.size())
    {
        return 0u;
    }
    return (static_cast<uint32_t>(data[off]) << 24) |
           (static_cast<uint32_t>(data[off + 1u]) << 16) |
           (static_cast<uint32_t>(data[off + 2u]) << 8) |
           static_cast<uint32_t>(data[off + 3u]);
}

[[nodiscard]] int32_t read_be32s(std::span<const uint8_t> data, std::size_t off) noexcept
{
    return static_cast<int32_t>(read_be32(data, off));
}

[[nodiscard]] std::span<const uint8_t> block_bytes(std::span<const uint8_t> image,
                                                   uint32_t block,
                                                   uint32_t sector_count) noexcept
{
    if (block >= sector_count)
    {
        return {};
    }
    const std::size_t off = static_cast<std::size_t>(block) *
                            static_cast<std::size_t>(k_adf_sector_bytes);
    if (off + static_cast<std::size_t>(k_adf_sector_bytes) > image.size())
    {
        return {};
    }
    return image.subspan(off, static_cast<std::size_t>(k_adf_sector_bytes));
}

[[nodiscard]] std::string bcpl_component(std::span<const uint8_t> block)
{
    if (block.size() <= static_cast<std::size_t>(k_off_name))
    {
        return {};
    }
    unsigned len = block[k_off_name];
    if (len > k_name_chars)
    {
        len = k_name_chars;
    }
    const std::size_t avail =
        block.size() - (static_cast<std::size_t>(k_off_name) + 1u);
    if (static_cast<std::size_t>(len) > avail)
    {
        len = static_cast<unsigned>(avail);
    }
    std::string out;
    out.reserve(len);
    for (unsigned i = 0; i < len; ++i)
    {
        const uint8_t c = block[static_cast<std::size_t>(k_off_name) + 1u + i];
        if (c == 0u)
        {
            break;
        }
        if (c < 0x20u || c > 0x7Eu)
        {
            continue;
        }
        if (c == static_cast<uint8_t>('/') || c == static_cast<uint8_t>('\\') ||
            c == static_cast<uint8_t>(':'))
        {
            out.push_back('_');
            continue;
        }
        out.push_back(static_cast<char>(c));
    }
    return out;
}

[[nodiscard]] std::string join_path(const std::string& prefix, const std::string& name)
{
    const std::string leaf = name.empty() ? std::string("unnamed") : name;
    if (prefix.empty())
    {
        return leaf;
    }
    return prefix + "/" + leaf;
}

void walk_chain(std::span<const uint8_t> image, uint32_t sector_count, uint32_t start,
                const std::string& prefix, std::vector<amiga_file>& out,
                std::unordered_set<uint32_t>& seen, int& visits);

void walk_hash_table(std::span<const uint8_t> image, uint32_t sector_count,
                     std::span<const uint8_t> dir_blk, const std::string& prefix,
                     std::vector<amiga_file>& out, std::unordered_set<uint32_t>& seen,
                     int& visits)
{
    uint32_t ht = read_be32(dir_blk, k_off_ht_size);
    if (ht == 0u || ht > k_adf_ht_size)
    {
        ht = k_adf_ht_size;
    }
    for (uint32_t i = 0; i < ht; ++i)
    {
        if (visits >= k_max_visits)
        {
            return;
        }
        const std::size_t off =
            static_cast<std::size_t>(k_off_table) + static_cast<std::size_t>(i) * 4u;
        const uint32_t key = read_be32(dir_blk, off);
        if (key != 0u)
        {
            walk_chain(image, sector_count, key, prefix, out, seen, visits);
        }
    }
}

void walk_chain(std::span<const uint8_t> image, uint32_t sector_count, uint32_t start,
                const std::string& prefix, std::vector<amiga_file>& out,
                std::unordered_set<uint32_t>& seen, int& visits)
{
    uint32_t block = start;
    while (block != 0u)
    {
        if (visits >= k_max_visits)
        {
            return;
        }
        ++visits;
        if (!seen.insert(block).second)
        {
            return;
        }
        if (out.size() >= static_cast<std::size_t>(sector_count))
        {
            return;
        }
        const std::span<const uint8_t> blk = block_bytes(image, block, sector_count);
        if (blk.size() != static_cast<std::size_t>(k_adf_sector_bytes))
        {
            return;
        }
        const int32_t typ = read_be32s(blk, k_off_type);
        const int32_t sec = read_be32s(blk, k_off_sec_type);
        const uint32_t chain = read_be32(blk, k_off_hash_chain);
        if (typ != k_t_header)
        {
            block = chain;
            continue;
        }
        if (sec == k_amiga_st_file)
        {
            amiga_file f{};
            f.name = bcpl_component(blk);
            f.path = join_path(prefix, f.name);
            f.header_block = block;
            f.parent_block = read_be32(blk, k_off_parent);
            f.byte_size = read_be32(blk, k_off_byte_size);
            f.is_dir = false;
            out.push_back(std::move(f));
        }
        else if (sec == k_amiga_st_userdir)
        {
            amiga_file f{};
            f.name = bcpl_component(blk);
            f.path = join_path(prefix, f.name);
            f.header_block = block;
            f.parent_block = read_be32(blk, k_off_parent);
            f.byte_size = 0;
            f.is_dir = true;
            const std::string child_prefix = f.path;
            out.push_back(std::move(f));
            walk_hash_table(image, sector_count, blk, child_prefix, out, seen, visits);
        }
        block = chain;
    }
}

[[nodiscard]] uint32_t data_key(std::span<const uint8_t> hdr, uint32_t i) noexcept
{
    /* Keys stored backwards: i=0 is the first data block (slot 71). */
    const std::size_t off = static_cast<std::size_t>(k_off_table) +
                            static_cast<std::size_t>(k_adf_ht_size - 1u - i) * 4u;
    return read_be32(hdr, off);
}

} /* namespace */

std::string amiga_host_filename(const amiga_file& file)
{
    std::string name = file.path.empty() ? file.name : file.path;
    for (char& c : name)
    {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' ||
            c == '<' || c == '>' || c == '|')
        {
            c = '_';
        }
    }
    if (name.empty())
    {
        name = "unnamed";
    }
    return name;
}

amiga_disk parse_adf(std::span<const uint8_t> image)
{
    amiga_disk disk{};
    const uint32_t sectors = adf_sector_count_for_size(image.size());
    if (sectors == 0u || image.size() < 4u)
    {
        return disk;
    }
    if (image[0] != static_cast<uint8_t>('D') ||
        image[1] != static_cast<uint8_t>('O') ||
        image[2] != static_cast<uint8_t>('S') || image[3] > 5u)
    {
        return disk;
    }

    const uint32_t root = adf_root_block(sectors);
    const std::span<const uint8_t> rblk = block_bytes(image, root, sectors);
    if (rblk.size() != static_cast<std::size_t>(k_adf_sector_bytes))
    {
        return disk;
    }
    if (read_be32s(rblk, k_off_type) != k_t_header)
    {
        return disk;
    }
    if (read_be32s(rblk, k_off_sec_type) != k_amiga_st_root)
    {
        return disk;
    }
    if (read_be32(rblk, k_off_ht_size) != k_adf_ht_size)
    {
        return disk;
    }

    disk.present = true;
    disk.dos_type = image[3];
    disk.ffs = amiga_dos_is_ffs(disk.dos_type);
    disk.volume_name = bcpl_component(rblk);
    disk.root_block = root;
    disk.sector_count = sectors;

    std::unordered_set<uint32_t> seen;
    seen.reserve(64);
    seen.insert(root);
    int visits = 0;
    walk_hash_table(image, sectors, rblk, std::string{}, disk.entries, seen, visits);
    return disk;
}

std::vector<uint8_t> read_amiga_file(std::span<const uint8_t> image,
                                     const amiga_disk& disk, const amiga_file& file)
{
    std::vector<uint8_t> payload;
    if (file.is_dir || file.header_block == 0u)
    {
        return payload;
    }

    uint32_t sectors = disk.sector_count;
    if (sectors == 0u)
    {
        sectors = adf_sector_count_for_size(image.size());
    }
    if (sectors == 0u)
    {
        return payload;
    }

    const std::span<const uint8_t> first =
        block_bytes(image, file.header_block, sectors);
    if (first.size() != static_cast<std::size_t>(k_adf_sector_bytes))
    {
        return payload;
    }
    const int32_t sec = read_be32s(first, k_off_sec_type);
    if (sec == k_amiga_st_userdir || sec == k_amiga_st_root)
    {
        return payload;
    }

    const uint32_t want = read_be32(first, k_off_byte_size);
    if (want == 0u)
    {
        return payload;
    }
    payload.reserve(want);

    const bool ffs = disk.ffs;
    std::unordered_set<uint32_t> meta_seen;
    std::unordered_set<uint32_t> data_seen;
    meta_seen.reserve(8);
    data_seen.reserve(16);

    uint32_t blk = file.header_block;
    uint32_t remaining = want;
    for (int step = 0; step < k_max_visits && remaining > 0u; ++step)
    {
        if (blk == 0u || !meta_seen.insert(blk).second)
        {
            break;
        }
        const std::span<const uint8_t> hdr = block_bytes(image, blk, sectors);
        if (hdr.size() != static_cast<std::size_t>(k_adf_sector_bytes))
        {
            break;
        }
        const int32_t typ = read_be32s(hdr, k_off_type);
        if (typ != k_t_header && typ != k_t_list)
        {
            break;
        }

        uint32_t high = read_be32(hdr, k_off_high_seq);
        if (high > k_adf_ht_size)
        {
            high = k_adf_ht_size;
        }
        for (uint32_t i = 0; i < high && remaining > 0u; ++i)
        {
            const uint32_t key = data_key(hdr, i);
            if (key == 0u || !data_seen.insert(key).second)
            {
                continue;
            }
            const std::span<const uint8_t> data = block_bytes(image, key, sectors);
            if (data.size() != static_cast<std::size_t>(k_adf_sector_bytes))
            {
                continue;
            }
            if (ffs)
            {
                const uint32_t n =
                    remaining < k_adf_sector_bytes ? remaining : k_adf_sector_bytes;
                payload.insert(payload.end(), data.begin(),
                               data.begin() + static_cast<std::ptrdiff_t>(n));
                remaining -= n;
            }
            else
            {
                if (read_be32s(data, k_off_type) != k_t_data)
                {
                    continue;
                }
                uint32_t n = read_be32(data, k_off_data_size);
                if (n > k_ofs_data_payload)
                {
                    n = k_ofs_data_payload;
                }
                if (n > remaining)
                {
                    n = remaining;
                }
                payload.insert(payload.end(),
                               data.begin() + static_cast<std::ptrdiff_t>(24),
                               data.begin() + static_cast<std::ptrdiff_t>(24u + n));
                remaining -= n;
            }
        }
        blk = read_be32(hdr, k_off_extension);
    }

    if (payload.size() > want)
    {
        payload.resize(want);
    }
    return payload;
}

} /* namespace dumpfloppy */
