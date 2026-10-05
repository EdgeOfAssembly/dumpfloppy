/**
 * @file cpm.cpp
 * @brief CP/M 2.2 directory scan. Names only; payloads stay on the disk.
 */
#include "dumpfloppy/cpm.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace dumpfloppy
{
namespace
{

bool cpm_name_byte(uint8_t b)
{
    return b == static_cast<uint8_t>(' ') ||
           (b >= static_cast<uint8_t>('0') && b <= static_cast<uint8_t>('9')) ||
           (b >= static_cast<uint8_t>('A') && b <= static_cast<uint8_t>('Z'));
}

bool cpm_slot(std::span<const uint8_t> image, std::size_t off)
{
    if (off + 32u > image.size())
    {
        return false;
    }
    const uint8_t user = image[off];
    const bool deleted = user == 0xE5u;
    if (!deleted && user > 0x0Fu)
    {
        return false;
    }
    bool letter = false;
    for (unsigned i = 1u; i <= 11u; ++i)
    {
        const uint8_t b = image[off + i];
        if (!cpm_name_byte(b))
        {
            return false;
        }
        if (b >= static_cast<uint8_t>('A') && b <= static_cast<uint8_t>('Z'))
        {
            letter = true;
        }
    }
    if (!letter || image[off + 1u] == static_cast<uint8_t>(' '))
    {
        return false;
    }
    /* CP/M 2.2 keeps the S1 byte at 0. Extent numbers stay in one byte. */
    if (image[off + 12u] > 0x1Fu || image[off + 13u] != 0u || image[off + 14u] > 0x1Fu)
    {
        return false;
    }
    return true;
}

std::string cpm_display_name(std::span<const uint8_t> slot)
{
    std::string base;
    std::string ext;
    base.reserve(8);
    ext.reserve(3);
    for (unsigned i = 1u; i <= 8u; ++i)
    {
        base.push_back(static_cast<char>(slot[i]));
    }
    for (unsigned i = 9u; i <= 11u; ++i)
    {
        ext.push_back(static_cast<char>(slot[i]));
    }
    while (!base.empty() && base.back() == ' ')
    {
        base.pop_back();
    }
    while (!ext.empty() && ext.back() == ' ')
    {
        ext.pop_back();
    }
    if (ext.empty())
    {
        return base;
    }
    return base + "." + ext;
}

uint32_t cpm_extent_bytes(std::span<const uint8_t> slot)
{
    const unsigned logical =
        static_cast<unsigned>(slot[14]) * 32u + static_cast<unsigned>(slot[12]);
    const unsigned records = slot[15];
    return static_cast<uint32_t>(logical) * 16384u +
           static_cast<uint32_t>(records) * 128u;
}

} /* namespace */

std::vector<dir_entry> list_cpm_directory(std::span<const uint8_t> image)
{
    std::size_t best_off = 0;
    std::size_t best_len = 0;
    std::size_t off = 0;
    while (off + 32u <= image.size())
    {
        if (!cpm_slot(image, off))
        {
            off += 32u;
            continue;
        }
        const std::size_t start = off;
        while (off + 32u <= image.size() && cpm_slot(image, off))
        {
            off += 32u;
        }
        const std::size_t len = (off - start) / 32u;
        if (len > best_len)
        {
            best_off = start;
            best_len = len;
        }
    }
    if (best_len < 2u)
    {
        return {};
    }

    struct grouped
    {
        std::string key{};
        dir_entry entry{};
        uint32_t extent = 0;
    };
    std::vector<grouped> groups;
    for (std::size_t n = 0; n < best_len; ++n)
    {
        const std::size_t slot_off = best_off + n * 32u;
        const std::span<const uint8_t> slot = image.subspan(slot_off, 32u);
        std::string key(reinterpret_cast<const char*>(slot.data() + 1), 11u);
        const bool deleted = slot[0] == 0xE5u;
        const uint32_t extent =
            static_cast<uint32_t>(slot[14]) * 32u + static_cast<uint32_t>(slot[12]);
        const uint32_t bytes = cpm_extent_bytes(slot);
        grouped* found = nullptr;
        for (grouped& g : groups)
        {
            if (g.key == key)
            {
                found = &g;
                break;
            }
        }
        if (found == nullptr)
        {
            grouped g;
            g.key = std::move(key);
            g.entry.name_83 = cpm_display_name(slot);
            g.entry.path = g.entry.name_83;
            g.entry.deleted = deleted;
            g.entry.size = bytes;
            g.entry.dir_slot_off = slot_off;
            g.entry.type = "CP/M";
            g.extent = extent;
            groups.push_back(std::move(g));
            continue;
        }
        if (!deleted)
        {
            found->entry.deleted = false;
        }
        if (extent >= found->extent)
        {
            found->extent = extent;
            found->entry.size = bytes;
        }
    }

    std::vector<dir_entry> out;
    out.reserve(groups.size());
    for (grouped& g : groups)
    {
        out.push_back(std::move(g.entry));
    }
    return out;
}

} /* namespace dumpfloppy */
