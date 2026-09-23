/**
 * @file trd.cpp
 * @brief TR-DOS disk-info + directory parse and sector-chain extract.
 */
#include "dumpfloppy/trd.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <vector>

namespace dumpfloppy
{
namespace
{

constexpr std::size_t k_info_off = k_trd_info_sector * k_trd_sector_bytes;
constexpr std::size_t k_entry_bytes = 16u;

const char* type_name_for(char type_char)
{
    switch (type_char)
    {
    case 'B':
        return "BASIC";
    case 'C':
        return "CODE";
    case 'D':
        return "DATA";
    case '#':
        return "PRINT";
    default:
        return "FILE";
    }
}

std::string strip_name(const char* raw8, bool deleted)
{
    std::string out;
    out.reserve(8u);
    if (deleted)
    {
        out.push_back('?');
        for (std::size_t i = 1; i < 8u; ++i)
        {
            const unsigned char c = static_cast<unsigned char>(raw8[i]);
            if (c == ' ' || c == 0u)
            {
                break;
            }
            out.push_back(static_cast<char>(c));
        }
        return out;
    }
    for (std::size_t i = 0; i < 8u; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(raw8[i]);
        if (c == ' ' || c == 0u)
        {
            break;
        }
        out.push_back(static_cast<char>(c));
    }
    return out;
}

void geometry_from_type(uint8_t disk_type, uint8_t& cyl, uint8_t& sides,
                        const char*& name)
{
    switch (disk_type)
    {
    case 0x16u:
        cyl = 80;
        sides = 2;
        name = "80 track DS (640K)";
        break;
    case 0x17u:
        cyl = 40;
        sides = 2;
        name = "40 track DS (320K)";
        break;
    case 0x18u:
        cyl = 80;
        sides = 1;
        name = "80 track SS (320K)";
        break;
    case 0x19u:
        cyl = 40;
        sides = 1;
        name = "40 track SS (160K)";
        break;
    default:
        cyl = 0;
        sides = 0;
        name = "unknown";
        break;
    }
}

} /* namespace */

bool is_trd_image(std::span<const uint8_t> data)
{
    const std::size_t n = data.size();
    if (n != k_trd_ds80_bytes && n != k_trd_320_bytes && n != k_trd_ss40_bytes)
    {
        return false;
    }
    if (n < k_info_off + 0xE8u)
    {
        return false;
    }
    const uint8_t id = data[k_info_off + 0xE7u];
    if (id == 0x10u)
    {
        return true;
    }
    const uint8_t first_sec = data[k_info_off + 0xE1u];
    const uint8_t disk_type = data[k_info_off + 0xE3u];
    return first_sec < k_trd_spt && disk_type >= 0x16u && disk_type <= 0x19u;
}

trd_disk parse_trd(std::span<const uint8_t> data)
{
    trd_disk disk{};
    if (!is_trd_image(data))
    {
        return disk;
    }

    const uint8_t disk_type = data[k_info_off + 0xE3u];
    const char* type_name = "unknown";
    geometry_from_type(disk_type, disk.cylinders, disk.sides, type_name);
    if (disk.cylinders == 0u)
    {
        return disk;
    }

    disk.present = true;
    disk.disk_type = disk_type;
    disk.disk_type_name = type_name;
    disk.first_free_sector = data[k_info_off + 0xE1u];
    disk.first_free_track = data[k_info_off + 0xE2u];
    disk.file_count = data[k_info_off + 0xE4u];
    disk.free_sectors = static_cast<uint16_t>(
        static_cast<unsigned>(data[k_info_off + 0xE5u]) |
        (static_cast<unsigned>(data[k_info_off + 0xE6u]) << 8));
    disk.deleted_count = data[k_info_off + 0xECu];

    char label[9]{};
    std::memcpy(label, data.data() + k_info_off + 0xEDu, 8u);
    disk.label = strip_name(label, false);

    const std::size_t dir_bytes = k_trd_dir_slots * k_entry_bytes;
    const std::size_t avail = std::min(dir_bytes, data.size());
    for (std::size_t i = 0; i + k_entry_bytes <= avail; i += k_entry_bytes)
    {
        const uint8_t* e = data.data() + i;
        if (e[0] == 0u)
        {
            break;
        }
        trd_file file{};
        file.deleted = (e[0] == 0x01u);
        char raw[8]{};
        std::memcpy(raw, e, 8u);
        file.name = strip_name(raw, file.deleted);
        file.type_char = static_cast<char>(e[8]);
        file.type_name = type_name_for(file.type_char);
        file.start = static_cast<uint16_t>(e[9]) |
                     (static_cast<uint16_t>(e[10]) << 8);
        file.byte_size = static_cast<uint16_t>(e[11]) |
                         (static_cast<uint16_t>(e[12]) << 8);
        file.sector_count = e[13];
        file.start_sector = e[14];
        file.start_track = e[15];
        disk.entries.push_back(std::move(file));
    }
    return disk;
}

std::string trd_host_filename(const trd_file& file)
{
    std::string stem = file.name.empty() ? std::string("FILE") : file.name;
    for (char& c : stem)
    {
        if (c == '/' || c == '\\' || c == '\0')
        {
            c = '_';
        }
    }
    const unsigned char ext = static_cast<unsigned char>(file.type_char);
    const char ext_ch = (ext <= 0x20u || ext >= 0x7Fu) ? 'X' : static_cast<char>(ext);
    stem.push_back('.');
    stem.push_back(ext_ch);
    return stem;
}

std::vector<uint8_t> read_trd_file(std::span<const uint8_t> image, const trd_file& file)
{
    const std::size_t start =
        (static_cast<std::size_t>(file.start_track) * k_trd_spt +
         static_cast<std::size_t>(file.start_sector)) *
        k_trd_sector_bytes;
    const std::size_t alloc =
        static_cast<std::size_t>(file.sector_count) * k_trd_sector_bytes;
    if (start >= image.size() || alloc == 0u)
    {
        return {};
    }
    const std::size_t n = std::min(alloc, image.size() - start);
    std::vector<uint8_t> out(image.begin() + static_cast<std::ptrdiff_t>(start),
                             image.begin() + static_cast<std::ptrdiff_t>(start + n));
    if (file.byte_size > 0u && file.byte_size < out.size())
    {
        out.resize(file.byte_size);
    }
    return out;
}

} /* namespace dumpfloppy */
