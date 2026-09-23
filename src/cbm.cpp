/**
 * @file cbm.cpp
 * @brief D64/D71/D81 CBMFS parser (BAM/header, directory chain, file chains).
 *
 * D81 also requires BAM 40/1 (DOS plus ones-complement) so IBM 800K FAT
 * with a coincidental `'D'` at 40/0 is not treated as 1581.
 */
#include "dumpfloppy/cbm.hpp"

#include <unordered_set>
#include <utility>

namespace dumpfloppy
{
namespace
{

constexpr uint8_t k_petscii_pad = 0xA0u;
constexpr uint8_t k_type_closed = 0x80u;
constexpr uint8_t k_type_locked = 0x40u;
constexpr uint8_t k_type_kind_mask = 0x0Fu;
constexpr unsigned k_dir_slots = 8u;
constexpr unsigned k_dirent_bytes = 32u;
constexpr unsigned k_name_bytes = 16u;
constexpr uint8_t k_d64_name_off = 0x90u;
constexpr uint8_t k_d64_id_off = 0xA2u;
constexpr uint8_t k_d64_type_off = 0xA5u;
constexpr uint8_t k_d81_name_off = 4u;
constexpr uint8_t k_d81_id_off = 22u;
constexpr uint8_t k_d81_type_off = 25u;

[[nodiscard]] uint16_t pack_ts(uint8_t track, uint8_t sector) noexcept
{
    return static_cast<uint16_t>((static_cast<uint16_t>(track) << 8) |
                                 static_cast<uint16_t>(sector));
}

[[nodiscard]] cbm_media media_or_d64(std::size_t n) noexcept
{
    const cbm_media inferred = cbm_media_from_size(n);
    return inferred == cbm_media::unknown ? cbm_media::d64 : inferred;
}

[[nodiscard]] bool sector_in_image(std::span<const uint8_t> image, std::size_t off) noexcept
{
    if (off == static_cast<std::size_t>(-1))
    {
        return false;
    }
    return off <= image.size() &&
           (image.size() - off) >= static_cast<std::size_t>(k_d64_sector_bytes);
}

[[nodiscard]] cbm_file_kind kind_from_type(uint8_t type_byte) noexcept
{
    switch (type_byte & k_type_kind_mask)
    {
    case 0:
        return cbm_file_kind::del;
    case 1:
        return cbm_file_kind::seq;
    case 2:
        return cbm_file_kind::prg;
    case 3:
        return cbm_file_kind::usr;
    case 4:
        return cbm_file_kind::rel;
    default:
        return cbm_file_kind::other;
    }
}

[[nodiscard]] bool petscii_name_blank(std::span<const uint8_t> name) noexcept
{
    for (const uint8_t b : name)
    {
        if (b != 0u && b != k_petscii_pad && b != static_cast<uint8_t>(' '))
        {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool dos_is_1541(uint8_t dos) noexcept
{
    return dos == static_cast<uint8_t>('A') || dos == 0u;
}

[[nodiscard]] bool dos_is_1581(uint8_t dos) noexcept
{
    return dos == static_cast<uint8_t>('D') || dos == 0u;
}

[[nodiscard]] bool bam_looks_cbmfs(std::span<const uint8_t> bam, cbm_media media) noexcept
{
    if (bam.size() < static_cast<std::size_t>(k_d64_sector_bytes))
    {
        return false;
    }
    if (!dos_is_1541(bam[2]))
    {
        return false;
    }
    return cbm_ts_valid(media, bam[0], bam[1]);
}

[[nodiscard]] bool header_looks_1581(std::span<const uint8_t> hdr) noexcept
{
    if (hdr.size() < static_cast<std::size_t>(k_d64_sector_bytes))
    {
        return false;
    }
    if (!dos_is_1581(hdr[2]))
    {
        return false;
    }
    /* D81.TXT: byte 3 is $00. A lone 'D' in IBM 800K data is not a header. */
    if (hdr[3] != 0u)
    {
        return false;
    }
    return cbm_ts_valid(cbm_media::d81, hdr[0], hdr[1]);
}

/**
 * 1581 BAM at 40/1: DOS version plus its ones-complement ($44/$BB).
 * IBM 800K FAT can coincidentally hold 40/3/'D' at 40/0; this pair does not.
 */
[[nodiscard]] bool bam_looks_1581(std::span<const uint8_t> image, uint8_t dos) noexcept
{
    const std::size_t off = cbm_offset(cbm_media::d81, k_d81_header_track, 1u);
    if (!sector_in_image(image, off))
    {
        return false;
    }
    const std::span<const uint8_t> bam =
        image.subspan(off, k_d64_sector_bytes);
    if (bam[2] != dos)
    {
        return false;
    }
    if (bam[3] != static_cast<uint8_t>(~dos))
    {
        return false;
    }
    return cbm_ts_valid(cbm_media::d81, bam[0], bam[1]);
}

void mark_present(cbm_disk& disk, cbm_media media)
{
    disk.present = true;
    disk.media = media;
    disk.media_name = cbm_media_name(media);
}

void parse_dir_slot(std::span<const uint8_t> slot, std::vector<cbm_file>& out)
{
    if (slot.size() < k_dirent_bytes)
    {
        return;
    }
    const uint8_t type_byte = slot[2];
    const uint8_t first_t = slot[3];
    const uint8_t first_s = slot[4];
    const std::span<const uint8_t> raw_name = slot.subspan(5, k_name_bytes);
    if ((type_byte & k_type_kind_mask) == 0u && first_t == 0u &&
        petscii_name_blank(raw_name))
    {
        return;
    }

    cbm_file f{};
    f.type_byte = type_byte;
    f.kind = kind_from_type(type_byte);
    f.closed = (type_byte & k_type_closed) != 0u;
    f.locked = (type_byte & k_type_locked) != 0u;
    f.deleted = !f.closed;
    f.first_track = first_t;
    f.first_sector = first_s;
    f.name = petscii_to_ascii(raw_name);
    f.size_sectors = static_cast<uint16_t>(
        static_cast<uint16_t>(slot[30]) |
        (static_cast<uint16_t>(slot[31]) << 8));
    out.push_back(std::move(f));
}

void walk_directory(std::span<const uint8_t> image, cbm_media media, uint8_t track,
                    uint8_t sector, std::vector<cbm_file>& out)
{
    std::unordered_set<uint16_t> seen;
    seen.reserve(32);
    for (int step = 0; step < k_cbm_max_chain; ++step)
    {
        if (track == 0u)
        {
            return;
        }
        if (!cbm_ts_valid(media, track, sector))
        {
            return;
        }
        const uint16_t key = pack_ts(track, sector);
        if (!seen.insert(key).second)
        {
            return;
        }
        const std::size_t off = cbm_offset(media, track, sector);
        if (!sector_in_image(image, off))
        {
            return;
        }
        const std::span<const uint8_t> sec = image.subspan(off, k_d64_sector_bytes);
        const uint8_t next_t = sec[0];
        const uint8_t next_s = sec[1];
        for (unsigned i = 0; i < k_dir_slots; ++i)
        {
            parse_dir_slot(sec.subspan(i * k_dirent_bytes, k_dirent_bytes), out);
        }
        track = next_t;
        sector = next_s;
    }
}

cbm_disk parse_1541_style(std::span<const uint8_t> image, cbm_media media)
{
    cbm_disk disk{};
    const std::size_t bam_off = cbm_offset(media, k_d64_bam_track, 0u);
    if (!sector_in_image(image, bam_off))
    {
        return disk;
    }
    const std::span<const uint8_t> bam =
        image.subspan(bam_off, k_d64_sector_bytes);
    if (!bam_looks_cbmfs(bam, media))
    {
        return disk;
    }

    mark_present(disk, media);
    disk.dir_track = bam[0];
    disk.dir_sector = bam[1];
    disk.dos_version = bam[2];
    disk.disk_name = petscii_to_ascii(bam.subspan(k_d64_name_off, k_name_bytes));
    disk.disk_id = petscii_to_ascii(bam.subspan(k_d64_id_off, 2u));
    disk.dos_type = petscii_to_ascii(bam.subspan(k_d64_type_off, 2u));
    walk_directory(image, media, disk.dir_track, disk.dir_sector, disk.entries);
    return disk;
}

cbm_disk parse_1581(std::span<const uint8_t> image)
{
    cbm_disk disk{};
    const std::size_t hdr_off = cbm_offset(cbm_media::d81, k_d81_header_track, 0u);
    if (!sector_in_image(image, hdr_off))
    {
        return disk;
    }
    const std::span<const uint8_t> hdr =
        image.subspan(hdr_off, k_d64_sector_bytes);
    if (!header_looks_1581(hdr))
    {
        return disk;
    }
    if (!bam_looks_1581(image, hdr[2]))
    {
        return disk;
    }

    mark_present(disk, cbm_media::d81);
    disk.dir_track = hdr[0];
    disk.dir_sector = hdr[1];
    disk.dos_version = hdr[2];
    disk.disk_name = petscii_to_ascii(hdr.subspan(k_d81_name_off, k_name_bytes));
    disk.disk_id = petscii_to_ascii(hdr.subspan(k_d81_id_off, 2u));
    disk.dos_type = petscii_to_ascii(hdr.subspan(k_d81_type_off, 2u));
    walk_directory(image, cbm_media::d81, disk.dir_track, disk.dir_sector,
                   disk.entries);
    return disk;
}

} /* namespace */

std::string petscii_to_ascii(std::span<const uint8_t> petscii)
{
    std::string out;
    out.reserve(petscii.size());
    for (const uint8_t b : petscii)
    {
        if (b == 0u || b == k_petscii_pad)
        {
            break;
        }
        if (b >= 0xC1u && b <= 0xDAu)
        {
            out.push_back(static_cast<char>(b - 0x80u));
            continue;
        }
        if (b >= 0x20u && b <= 0x5Fu)
        {
            out.push_back(static_cast<char>(b));
        }
    }
    while (!out.empty() && out.back() == ' ')
    {
        out.pop_back();
    }
    return out;
}

const char* cbm_file_kind_name(cbm_file_kind kind) noexcept
{
    switch (kind)
    {
    case cbm_file_kind::del:
        return "DEL";
    case cbm_file_kind::seq:
        return "SEQ";
    case cbm_file_kind::prg:
        return "PRG";
    case cbm_file_kind::usr:
        return "USR";
    case cbm_file_kind::rel:
        return "REL";
    case cbm_file_kind::other:
    default:
        return "???";
    }
}

const char* cbm_file_kind_ext(cbm_file_kind kind) noexcept
{
    switch (kind)
    {
    case cbm_file_kind::seq:
        return ".seq";
    case cbm_file_kind::usr:
        return ".usr";
    case cbm_file_kind::rel:
        return ".rel";
    case cbm_file_kind::del:
        return ".del";
    case cbm_file_kind::other:
        return ".cbm";
    case cbm_file_kind::prg:
    default:
        return ".prg";
    }
}

std::string cbm_host_filename(const cbm_file& file)
{
    std::string name = file.name;
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
    name += cbm_file_kind_ext(file.kind);
    return name;
}

cbm_disk parse_cbmfs(std::span<const uint8_t> image, cbm_media media)
{
    switch (media)
    {
    case cbm_media::d64:
    case cbm_media::d71:
        return parse_1541_style(image, media);
    case cbm_media::d81:
        return parse_1581(image);
    case cbm_media::unknown:
    default:
        return {};
    }
}

cbm_disk parse_cbmfs(std::span<const uint8_t> image)
{
    return parse_cbmfs(image, media_or_d64(image.size()));
}

cbm_disk parse_d64(std::span<const uint8_t> image)
{
    if (!is_d64_35_size(image.size()))
    {
        return {};
    }
    return parse_cbmfs(image, cbm_media::d64);
}

cbm_disk parse_d71(std::span<const uint8_t> image)
{
    if (!is_d71_size(image.size()))
    {
        return {};
    }
    return parse_cbmfs(image, cbm_media::d71);
}

cbm_disk parse_d81(std::span<const uint8_t> image)
{
    if (!is_d81_size(image.size()))
    {
        return {};
    }
    return parse_cbmfs(image, cbm_media::d81);
}

cbm_disk parse_cbm_image(std::span<const uint8_t> image)
{
    const cbm_media media = cbm_media_from_size(image.size());
    if (media == cbm_media::unknown)
    {
        return {};
    }
    return parse_cbmfs(image, media);
}

std::vector<uint8_t> read_cbm_file(std::span<const uint8_t> image, cbm_media media,
                                   uint8_t track, uint8_t sector)
{
    std::vector<uint8_t> payload;
    if (track == 0u)
    {
        return payload;
    }

    std::unordered_set<uint16_t> seen;
    seen.reserve(64);
    payload.reserve(254u);
    for (int step = 0; step < k_cbm_max_chain; ++step)
    {
        if (!cbm_ts_valid(media, track, sector))
        {
            return payload;
        }
        const uint16_t key = pack_ts(track, sector);
        if (!seen.insert(key).second)
        {
            return payload;
        }
        const std::size_t off = cbm_offset(media, track, sector);
        if (!sector_in_image(image, off))
        {
            return payload;
        }
        const std::span<const uint8_t> sec =
            image.subspan(off, k_d64_sector_bytes);
        const uint8_t next_t = sec[0];
        const uint8_t next_s = sec[1];
        if (next_t == 0u)
        {
            /* Last sector: S is the last used byte index (D64.TXT: $02–$S). */
            if (next_s >= 2u)
            {
                const std::size_t last =
                    (next_s < k_d64_sector_bytes)
                        ? static_cast<std::size_t>(next_s)
                        : (static_cast<std::size_t>(k_d64_sector_bytes) - 1u);
                payload.insert(payload.end(), sec.begin() + 2, sec.begin() + last + 1);
            }
            return payload;
        }
        payload.insert(payload.end(), sec.begin() + 2, sec.begin() + k_d64_sector_bytes);
        track = next_t;
        sector = next_s;
    }
    return payload;
}

std::vector<uint8_t> read_cbm_file(std::span<const uint8_t> image, uint8_t track,
                                   uint8_t sector)
{
    return read_cbm_file(image, media_or_d64(image.size()), track, sector);
}

std::vector<uint8_t> read_cbm_file(std::span<const uint8_t> image, cbm_media media,
                                   const cbm_file& file)
{
    return read_cbm_file(image, media, file.first_track, file.first_sector);
}

std::vector<uint8_t> read_cbm_file(std::span<const uint8_t> image, const cbm_file& file)
{
    return read_cbm_file(image, media_or_d64(image.size()), file);
}

} /* namespace dumpfloppy */
