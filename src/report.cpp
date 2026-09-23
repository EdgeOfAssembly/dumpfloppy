/**
 * @file report.cpp
 * @brief Colourful floppy secret dump. Deleted names use TUI light-red + bold white.
 */
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/amiga.hpp"
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/apple.hpp"
#include "dumpfloppy/bpb.hpp"
#include "dumpfloppy/cbm.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/format.h"
#include "dumpfloppy/format_registry.hpp"
#include "dumpfloppy/geometry.hpp"
#include "dumpfloppy/ibm_mfm.hpp"
#include "dumpfloppy/foreign.hpp"
#include "dumpfloppy/trd.hpp"
#include "dumpfloppy/util.hpp"
#include "dumpfloppy/version.hpp"

#include <tui/ansi.h>

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace dumpfloppy
{
namespace
{

void put_style(std::ostream& out, bool color, const char* s)
{
    if (color)
    {
        out << s;
    }
}

void section(std::ostream& out, bool color, std::string_view title)
{
    put_style(out, color, TUI_BOLD);
    put_style(out, color, TUI_CYAN);
    out << title;
    put_style(out, color, TUI_RESET);
    out << '\n';
}

void kv(std::ostream& out, std::string_view key, std::string_view value)
{
    out << "  " << std::left << std::setw(18) << key << value << '\n';
}

const char* container_name(container_kind k)
{
    switch (k)
    {
        case container_kind::ima_winimage:
            return "IMA (WinImage raw; same sector layout as IMG)";
        case container_kind::img_raw:
            return "IMG (raw sector dump)";
        case container_kind::hxc_mfm:
            return "HxC MFM bitstream (.mfm)";
        case container_kind::box86f:
            return "86Box 86F surface (.86f)";
        case container_kind::d64_c64:
            return "D64 (Commodore 1541 CBMFS)";
        case container_kind::d71_c64:
            return "D71 (Commodore 1571 CBMFS)";
        case container_kind::d81_c64:
            return "D81 (Commodore 1581 CBMFS)";
        case container_kind::adf_amiga:
            return "ADF (Amiga OFS/FFS)";
        case container_kind::g64_c64:
            return "G64 (Commodore 1541 GCR-1541)";
        case container_kind::trd_spectrum:
            return "TRD (ZX Spectrum TR-DOS)";
        case container_kind::ipf_sps:
            return "IPF (SPS CAPS flux)";
        case container_kind::woz_apple:
            return "WOZ (Apple II Applesauce)";
        case container_kind::stx_atari:
            return "STX (Atari ST Pasti)";
        case container_kind::img2mg_apple:
            return "2IMG (Apple II prefix)";
        default:
            return "raw (not .img/.ima/.mfm/.86f/.d64/.d71/.d81/.adf/.g64/.trd/.ipf/.woz/.stx/.2mg)";
    }
}

const char* fat_name(fat_kind k)
{
    switch (k)
    {
        case fat_kind::fat12:
            return "FAT12";
        case fat_kind::fat16:
            return "FAT16";
        case fat_kind::fat32:
            return "FAT32";
        default:
            return "unknown";
    }
}

void hex_dump(std::ostream& out, std::span<const uint8_t> data)
{
    for (size_t i = 0; i < data.size(); i += 16u)
    {
        out << "  " << std::hex << std::setw(4) << std::setfill('0') << i << "  "
            << std::dec << std::setfill(' ');
        std::string ascii(16, ' ');
        const size_t row = std::min<size_t>(16u, data.size() - i);
        for (size_t j = 0; j < 16u; ++j)
        {
            if (j == 8u)
            {
                out << ' ';
            }
            if (j < row)
            {
                const uint8_t b = data[i + j];
                out << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<unsigned>(b) << ' ' << std::dec << std::setfill(' ');
                ascii[j] = (b >= 0x20u && b <= 0x7Eu) ? static_cast<char>(b) : '.';
            }
            else
            {
                out << "   ";
            }
        }
        out << " |" << ascii << "|\n";
    }
}

void emit_deleted_line(std::ostream& out, bool color, const std::string& line)
{
    if (color)
    {
        /* Deleted rows: no blink (AGENTS.md). */
        out << TUI_BG_BRIGHT_RED << TUI_WHITE << TUI_BOLD << line
            << TUI_RESET << '\n';
    }
    else
    {
        out << line << '\n';
    }
}

/**
 * Left-justify @p s into @p width characters, then two spaces of gap.
 * FAT12 8.3 names are at most 12 characters (`NAME.EXT`).
 */
std::string field(std::string_view s, size_t width)
{
    std::string out;
    out.reserve(width + 2u);
    const size_t n = std::min(s.size(), width);
    out.append(s.data(), n);
    if (n < width)
    {
        out.append(width - n, ' ');
    }
    out.append(2, ' ');
    return out;
}

/*
 * Inner widths = max(header, content); each field then adds 2 spaces.
 * Name 12, Attributes 10, Size 7 (floppy files), Cluster 7, Modified 19,
 * Type k_type_column_width (format label, default DATA), XXH64 16 hex.
 */
constexpr size_t k_w_mark = 1;
constexpr size_t k_w_name = 12;
constexpr size_t k_w_attr = 10;
constexpr size_t k_w_size = 7;
constexpr size_t k_w_cluster = 7;
constexpr size_t k_w_modified = 19;
constexpr size_t k_w_sum = 16;

std::string directory_header()
{
    std::ostringstream os;
    os << "  " << field(" ", k_w_mark) << field("Name", k_w_name)
       << field("Attributes", k_w_attr) << field("Size", k_w_size)
       << field("Cluster", k_w_cluster) << field("Modified", k_w_modified)
       << field("Type", k_type_column_width) << field("XXH64 Checksum", k_w_sum);
    return os.str();
}

std::string entry_line(const dir_entry& e)
{
    const char mark = e.deleted ? 'D' : ' ';
    const std::string modified =
        format_dos_date(e.write_date) + " " + format_dos_time(e.write_time);
    std::ostringstream os;
    os << "  " << field(std::string_view(&mark, 1), k_w_mark)
       << field(e.name_83, k_w_name) << field(format_attributes(e.attributes), k_w_attr)
       << field(std::to_string(e.size), k_w_size)
       << field(std::to_string(e.first_cluster), k_w_cluster)
       << field(modified, k_w_modified) << field(e.type, k_type_column_width)
       << field(e.size == 0u ? std::string_view{} : e.xxh64, k_w_sum);
    if (!e.notes.empty())
    {
        os << e.notes;
    }
    if (e.after_terminator)
    {
        os << "after-0x00";
    }
    return os.str();
}

constexpr size_t k_w_cbm_name = 16;
constexpr size_t k_w_cbm_type = 4;
constexpr size_t k_w_cbm_ts = 7;

std::string cbm_directory_header()
{
    std::ostringstream os;
    os << "  " << field(" ", k_w_mark) << field("Name", k_w_cbm_name)
       << field("Type", k_w_cbm_type) << field("Size", k_w_size)
       << field("T/S", k_w_cbm_ts) << field("XXH64 Checksum", k_w_sum);
    return os.str();
}

std::string cbm_entry_line(const cbm_file& e, uint32_t size, std::string_view sum)
{
    const char mark = e.deleted ? 'D' : ' ';
    std::ostringstream ts;
    ts << static_cast<unsigned>(e.first_track) << '/'
       << static_cast<unsigned>(e.first_sector);
    std::ostringstream os;
    os << "  " << field(std::string_view(&mark, 1), k_w_mark)
       << field(e.name, k_w_cbm_name) << field(cbm_file_kind_name(e.kind), k_w_cbm_type)
       << field(std::to_string(size), k_w_size) << field(ts.str(), k_w_cbm_ts)
       << field(size == 0u ? std::string_view{} : sum, k_w_sum);
    return os.str();
}

void write_cbm_sections(const analysis& a, std::ostream& out, const report_options& opt)
{
    const bool color = opt.color;
    section(out, color, "CBM VOLUME");
    kv(out, "Disk name", a.cbm.disk_name.empty() ? "(none)" : a.cbm.disk_name);
    kv(out, "Disk ID", a.cbm.disk_id.empty() ? "(none)" : a.cbm.disk_id);
    {
        std::ostringstream dosv;
        if (a.cbm.dos_version >= 0x20u && a.cbm.dos_version <= 0x7Eu)
        {
            dosv << static_cast<char>(a.cbm.dos_version);
        }
        else
        {
            dosv << "0x" << std::hex << std::uppercase
                 << static_cast<unsigned>(a.cbm.dos_version);
        }
        kv(out, "DOS version", dosv.str());
    }
    kv(out, "Media", a.cbm.media_name.empty() ? cbm_media_name(a.cbm.media)
                                              : a.cbm.media_name.c_str());
    kv(out, "DOS type", a.cbm.dos_type.empty() ? "(none)" : a.cbm.dos_type);
    {
        std::ostringstream ts;
        ts << static_cast<unsigned>(a.cbm.dir_track) << '/'
           << static_cast<unsigned>(a.cbm.dir_sector);
        kv(out, "Directory T/S", ts.str());
    }
    out << '\n';

    section(out, color, "DIRECTORY");
    put_style(out, color, TUI_BOLD);
    put_style(out, color, TUI_WHITE);
    out << cbm_directory_header();
    put_style(out, color, TUI_RESET);
    out << '\n';
    size_t shown = 0;
    size_t deleted_n = 0;
    for (const cbm_file& e : a.cbm.entries)
    {
        if (e.deleted)
        {
            ++deleted_n;
            if (!opt.show_deleted)
            {
                continue;
            }
        }
        const std::vector<uint8_t> payload =
            read_cbm_file(cbm_sector_bytes(a.image.bytes, a.cbm), e);
        const uint32_t size = static_cast<uint32_t>(payload.size());
        const std::string sum = payload.empty() ? std::string{} : xxh64_hex(payload);
        const std::string line = cbm_entry_line(e, size, sum);
        if (e.deleted)
        {
            emit_deleted_line(out, color, line);
        }
        else
        {
            out << line << '\n';
        }
        ++shown;
    }
    if (a.cbm.entries.empty())
    {
        out << "  (no CBMFS directory entries)\n";
    }
    out << "  " << shown << " entries shown, " << deleted_n << " deleted on disk\n";
    out << '\n';
}

constexpr size_t k_w_amiga_name = 24;
constexpr size_t k_w_amiga_type = 4;

std::string amiga_directory_header()
{
    std::ostringstream os;
    os << "  " << field(" ", k_w_mark) << field("Name", k_w_amiga_name)
       << field("Type", k_w_amiga_type) << field("Size", k_w_size)
       << field("XXH64 Checksum", k_w_sum);
    return os.str();
}

std::string amiga_entry_line(const amiga_file& e, uint32_t size, std::string_view sum)
{
    const char mark = ' ';
    const char* kind = e.is_dir ? "DIR" : "FILE";
    const std::string size_text = e.is_dir ? std::string{} : std::to_string(size);
    std::ostringstream os;
    os << "  " << field(std::string_view(&mark, 1), k_w_mark)
       << field(e.path.empty() ? e.name : e.path, k_w_amiga_name)
       << field(kind, k_w_amiga_type) << field(size_text, k_w_size)
       << field((e.is_dir || size == 0u) ? std::string_view{} : sum, k_w_sum);
    return os.str();
}

void write_amiga_sections(const analysis& a, std::ostream& out, const report_options& opt)
{
    const bool color = opt.color;
    section(out, color, "AMIGA VOLUME");
    kv(out, "Volume name",
       a.amiga.volume_name.empty() ? "(none)" : a.amiga.volume_name);
    {
        std::ostringstream dos;
        dos << "DOS\\" << static_cast<unsigned>(a.amiga.dos_type) << "  "
            << amiga_fs_name(a.amiga.ffs);
        kv(out, "DOS type", dos.str());
    }
    kv(out, "Filesystem", amiga_fs_name(a.amiga.ffs));
    kv(out, "Root block", std::to_string(a.amiga.root_block));
    kv(out, "Sectors", std::to_string(a.amiga.sector_count));
    out << '\n';

    section(out, color, "DIRECTORY");
    put_style(out, color, TUI_BOLD);
    put_style(out, color, TUI_WHITE);
    out << amiga_directory_header();
    put_style(out, color, TUI_RESET);
    out << '\n';
    size_t shown = 0;
    size_t dirs = 0;
    for (const amiga_file& e : a.amiga.entries)
    {
        if (e.is_dir)
        {
            ++dirs;
            out << amiga_entry_line(e, 0, {}) << '\n';
            ++shown;
            continue;
        }
        const std::vector<uint8_t> payload =
            read_amiga_file(a.image.bytes, a.amiga, e);
        const uint32_t size = static_cast<uint32_t>(payload.size());
        const std::string sum = payload.empty() ? std::string{} : xxh64_hex(payload);
        out << amiga_entry_line(e, size, sum) << '\n';
        ++shown;
    }
    if (a.amiga.entries.empty())
    {
        out << "  (no OFS/FFS directory entries)\n";
    }
    out << "  " << shown << " entries shown, " << dirs
        << " directories, deleted N/A\n";
    out << '\n';
}

constexpr size_t k_w_trd_name = 12;
constexpr size_t k_w_trd_type = 5;
constexpr size_t k_w_trd_ts = 7;

std::string trd_directory_header()
{
    std::ostringstream os;
    os << "  " << field(" ", k_w_mark) << field("Name", k_w_trd_name)
       << field("Type", k_w_trd_type) << field("Size", k_w_size)
       << field("T/S", k_w_trd_ts) << field("XXH64 Checksum", k_w_sum);
    return os.str();
}

std::string trd_entry_line(const trd_file& e, uint32_t size, std::string_view sum)
{
    const char mark = e.deleted ? 'D' : ' ';
    std::ostringstream ts;
    ts << static_cast<unsigned>(e.start_track) << '/'
       << static_cast<unsigned>(e.start_sector);
    std::ostringstream os;
    os << "  " << field(std::string_view(&mark, 1), k_w_mark)
       << field(e.name, k_w_trd_name) << field(e.type_name, k_w_trd_type)
       << field(std::to_string(size), k_w_size) << field(ts.str(), k_w_trd_ts)
       << field(size == 0u ? std::string_view{} : sum, k_w_sum);
    return os.str();
}

void write_trd_sections(const analysis& a, std::ostream& out, const report_options& opt)
{
    const bool color = opt.color;
    section(out, color, "TR-DOS VOLUME");
    kv(out, "Disk name", a.trd.label.empty() ? "(none)" : a.trd.label);
    kv(out, "Disk type", a.trd.disk_type_name.empty() ? "(none)" : a.trd.disk_type_name);
    kv(out, "Files", std::to_string(a.trd.file_count));
    kv(out, "Deleted", std::to_string(a.trd.deleted_count));
    kv(out, "Free sectors", std::to_string(a.trd.free_sectors));
    {
        std::ostringstream ts;
        ts << static_cast<unsigned>(a.trd.first_free_track) << '/'
           << static_cast<unsigned>(a.trd.first_free_sector);
        kv(out, "First free T/S", ts.str());
    }
    out << '\n';

    section(out, color, "DIRECTORY");
    put_style(out, color, TUI_BOLD);
    put_style(out, color, TUI_WHITE);
    out << trd_directory_header();
    put_style(out, color, TUI_RESET);
    out << '\n';
    size_t shown = 0;
    size_t deleted_n = 0;
    for (const trd_file& e : a.trd.entries)
    {
        if (e.deleted)
        {
            ++deleted_n;
            if (!opt.show_deleted)
            {
                continue;
            }
        }
        const std::vector<uint8_t> payload = read_trd_file(a.image.bytes, e);
        const uint32_t size = static_cast<uint32_t>(payload.size());
        const std::string sum = payload.empty() ? std::string{} : xxh64_hex(payload);
        const std::string line = trd_entry_line(e, size, sum);
        if (e.deleted)
        {
            emit_deleted_line(out, color, line);
        }
        else
        {
            out << line << '\n';
        }
        ++shown;
    }
    if (a.trd.entries.empty())
    {
        out << "  (no TR-DOS directory entries)\n";
    }
    out << "  " << shown << " entries shown, " << deleted_n << " deleted on disk\n";
    out << '\n';
}

void write_foreign_sections(const analysis& a, std::ostream& out,
                            const report_options& opt)
{
    const bool color = opt.color;
    section(out, color, "CONTAINER");
    kv(out, "Format", a.foreign.format);
    kv(out, "Platform", a.foreign.platform.empty() ? "(none)" : a.foreign.platform);
    if (!a.foreign.creator.empty())
    {
        kv(out, "Creator", a.foreign.creator);
    }
    if (a.foreign.file_id != 0u)
    {
        kv(out, "SPS id", std::to_string(a.foreign.file_id));
    }
    if (a.foreign.track_count != 0u)
    {
        kv(out, "Track images", std::to_string(a.foreign.track_count));
    }
    if (a.foreign.max_cylinder != 0u || a.foreign.max_head != 0u)
    {
        std::ostringstream os;
        os << a.foreign.min_cylinder << "–" << a.foreign.max_cylinder << " cyl, heads "
           << a.foreign.min_head << "–" << a.foreign.max_head;
        kv(out, "Geometry", os.str());
    }
    if (a.foreign.write_protected)
    {
        kv(out, "Write protect", "yes");
    }
    if (a.foreign.data_length != 0u)
    {
        kv(out, "Payload", std::to_string(a.foreign.data_length) + " bytes @ " +
                               std::to_string(a.foreign.data_offset));
    }
    if (!a.foreign.note.empty())
    {
        kv(out, "Note", a.foreign.note);
    }
    out << '\n';
}

constexpr size_t k_w_apple_name = 16;
constexpr size_t k_w_apple_type = 4;

std::string apple_directory_header()
{
    std::ostringstream os;
    os << "  " << field(" ", k_w_mark) << field("Name", k_w_apple_name)
       << field("Type", k_w_apple_type) << field("Size", k_w_size)
       << field("XXH64 Checksum", k_w_sum);
    return os.str();
}

std::string apple_entry_line(const apple_file& e, uint32_t size, std::string_view sum)
{
    const char mark = e.deleted ? 'D' : (e.locked ? '*' : ' ');
    std::ostringstream os;
    os << "  " << field(std::string_view(&mark, 1), k_w_mark)
       << field(e.name, k_w_apple_name) << field(e.type_name, k_w_apple_type)
       << field(std::to_string(size), k_w_size)
       << field(size == 0u ? std::string_view{} : sum, k_w_sum);
    return os.str();
}

void write_apple_sections(const analysis& a, std::ostream& out,
                          const report_options& opt)
{
    const bool color = opt.color;
    section(out, color, "APPLE VOLUME");
    kv(out, "Filesystem", a.apple.fs_name);
    kv(out, "Volume name",
       a.apple.volume_name.empty() ? "(none)" : a.apple.volume_name);
    if (a.apple.fs == apple_fs::dos33)
    {
        kv(out, "Volume #", std::to_string(a.apple.volume_number));
        kv(out, "Tracks", std::to_string(a.apple.tracks));
        kv(out, "Sectors/track", std::to_string(a.apple.sectors_per_track));
    }
    else if (a.apple.total_blocks != 0u)
    {
        kv(out, "Blocks", std::to_string(a.apple.total_blocks));
    }
    out << '\n';

    section(out, color, "DIRECTORY");
    put_style(out, color, TUI_BOLD);
    put_style(out, color, TUI_WHITE);
    out << apple_directory_header();
    put_style(out, color, TUI_RESET);
    out << '\n';
    size_t shown = 0;
    size_t deleted_n = 0;
    for (const apple_file& e : a.apple.entries)
    {
        if (e.deleted)
        {
            ++deleted_n;
            if (!opt.show_deleted)
            {
                continue;
            }
        }
        const std::vector<uint8_t> payload = read_apple_file(a.apple, e);
        const uint32_t size = static_cast<uint32_t>(payload.size());
        const std::string sum = payload.empty() ? std::string{} : xxh64_hex(payload);
        const std::string line = apple_entry_line(e, size, sum);
        if (e.deleted)
        {
            emit_deleted_line(out, color, line);
        }
        else
        {
            out << line << '\n';
        }
        ++shown;
    }
    if (a.apple.entries.empty())
    {
        out << "  (no Apple directory entries)\n";
    }
    out << "  " << shown << " entries shown, " << deleted_n << " deleted on disk\n";
    out << '\n';
}

} /* namespace */

void write_report(const analysis& a, std::ostream& out, const report_options& opt)
{
    const bool color = opt.color;
    out << k_program << " " << k_version << " — floppy image secrets\n\n";

    section(out, color, "IMAGE");
    kv(out, "Path", a.image.path.string());
    {
        std::ostringstream os;
        os << a.image.bytes.size() << " bytes";
        if (!a.image.size_geometry.media_name.empty())
        {
            os << "  (" << a.image.size_geometry.media_name << ")";
        }
        kv(out, "Size", os.str());
    }
    kv(out, "SHA-256", a.image.sha256);
    kv(out, "XXH64", a.image.xxh64);
    {
        std::string fmt;
        if (a.cbm.present)
        {
            const char* media = a.cbm.media_name.empty()
                                    ? cbm_media_name(a.cbm.media)
                                    : a.cbm.media_name.c_str();
            fmt = std::string("C64 ") + media + " / CBMFS";
        }
        else if (a.amiga.present)
        {
            fmt = std::string("AMIGA ADF / ") + amiga_fs_name(a.amiga.ffs);
        }
        else if (a.trd.present)
        {
            fmt = "ZX TRD / TR-DOS";
        }
        else if (a.apple.present)
        {
            if (a.foreign.kind == foreign_kind::img2mg)
            {
                fmt = std::string("APPLE 2IMG / ") + a.apple.fs_name;
            }
            else
            {
                fmt = std::string("APPLE ") + a.apple.fs_name;
            }
        }
        else if (a.foreign.present)
        {
            fmt = a.foreign.format;
        }
        else
        {
            fmt = identify_type(a.image.bytes, format_kind::disk_image);
        }
        kv(out, "Format", fmt);
    }
    kv(out, "Container", container_name(a.image.container));
    if (a.cbm.present)
    {
        const char* drive = "1541 D64";
        if (a.cbm.media == cbm_media::d71)
        {
            drive = "1571 D71";
        }
        else if (a.cbm.media == cbm_media::d81)
        {
            drive = "1581 D81";
        }
        else if (a.cbm.media == cbm_media::g64)
        {
            drive = "1541 G64";
        }
        kv(out, "Filesystem", std::string("CBMFS (Commodore ") + drive + ")");
    }
    else if (a.amiga.present)
    {
        kv(out, "Filesystem", std::string(amiga_fs_name(a.amiga.ffs)) + " (Amiga)");
    }
    else if (a.trd.present)
    {
        kv(out, "Filesystem", "TR-DOS");
    }
    else if (a.apple.present)
    {
        kv(out, "Filesystem", a.apple.fs_name);
    }
    else if (a.foreign.present)
    {
        kv(out, "Filesystem", a.foreign.platform.empty() ? "(flux)" : a.foreign.platform);
    }
    if (a.image.size_geometry.cylinders != 0u)
    {
        std::ostringstream os;
        os << a.image.size_geometry.cylinders << " cyl × "
           << a.image.size_geometry.heads << " heads × "
           << a.image.size_geometry.sectors_per_track << " spt × "
           << a.image.size_geometry.bytes_per_sector << " bps";
        kv(out, "Geometry(size)", os.str());
    }
    if (a.trailing_bytes != 0u)
    {
        kv(out, "Trailing", std::to_string(a.trailing_bytes) + " bytes past BPB volume");
    }
    if (a.truncated)
    {
        kv(out, "Truncated", "yes");
    }
    out << '\n';

    if (a.catalog.found)
    {
        section(out, color, "CATALOG");
        kv(out, "Title", a.catalog.title);
        kv(out, "Protection", a.catalog.protection);
        out << '\n';
    }

    if (a.flux.present)
    {
        section(out, color, "FLUX / MFM");
        kv(out, "Format", a.flux.format_name);
        if (a.flux.tracks != 0u)
        {
            std::ostringstream os;
            os << a.flux.tracks << " tracks × " << a.flux.sides << " sides, "
               << a.flux.rpm << " rpm, " << a.flux.bitrate_kbps << " kbps";
            kv(out, "Geometry", os.str());
        }
        kv(out, "IBM sectors", std::to_string(a.flux.sectors.size()));
        if (!a.flux.note.empty())
        {
            kv(out, "Note", a.flux.note);
        }
        out << '\n';

        section(out, color, "COPY PROTECTION");
        if (a.flux.protection.empty() && !a.catalog.found)
        {
            out << "  (none detected)\n";
        }
        for (const std::string& s : a.flux.protection)
        {
            out << "  * " << s << '\n';
        }
        out << '\n';

        if (!a.flux.sectors.empty())
        {
            section(out, color, "SECTORS");
            out << "  C   H  S   bytes  IDAM  DAM\n";
            std::vector<uint32_t> seen_sec;
            for (const ibm_sector& s : a.flux.sectors)
            {
                if (s.size_code == 2u && s.dam_crc_ok && s.idam_crc_ok &&
                    s.sector >= 1u && s.sector <= 9u)
                {
                    continue;
                }
                const uint32_t key = (static_cast<uint32_t>(s.cyl) << 16) |
                                     (static_cast<uint32_t>(s.head) << 8) | s.sector;
                bool dup = false;
                for (uint32_t k : seen_sec)
                {
                    if (k == key)
                    {
                        dup = true;
                        break;
                    }
                }
                if (dup)
                {
                    continue;
                }
                seen_sec.push_back(key);
                out << "  " << std::setw(3) << static_cast<unsigned>(s.cyl) << "  "
                    << static_cast<unsigned>(s.head) << "  "
                    << std::setw(2) << static_cast<unsigned>(s.sector) << "  "
                    << std::setw(5) << s.bytes << "  "
                    << (s.idam_crc_ok ? "ok  " : "BAD ") << "  "
                    << (s.has_dam ? (s.dam_crc_ok ? "ok" : "CRC BAD") : "none")
                    << '\n';
            }
            out << "  (standard 512-byte CRC-ok sectors omitted)\n\n";
        }
    }

    if (a.cbm.present)
    {
        write_cbm_sections(a, out, opt);
    }
    else if (a.amiga.present)
    {
        write_amiga_sections(a, out, opt);
    }
    else if (a.trd.present)
    {
        write_trd_sections(a, out, opt);
    }
    else if (a.apple.present)
    {
        if (a.foreign.present)
        {
            write_foreign_sections(a, out, opt);
        }
        write_apple_sections(a, out, opt);
    }
    else if (a.foreign.present)
    {
        write_foreign_sections(a, out, opt);
    }
    else
    {
    section(out, color, "BOOT");
    kv(out, "Jump", describe_jump(a.bpb.jump));
    kv(out, "OEM", a.bpb.oem.empty() ? "(none)" : a.bpb.oem);
    kv(out, "55 AA", a.boot.has_aa55 ? "yes (offset 510)" : "no");
    kv(out, "Boot class", a.boot.kind_text);
    if (!a.boot.strings.empty())
    {
        out << "  Strings in boot sector:\n";
        for (const std::string& s : a.boot.strings)
        {
            out << "    \"" << s << "\"\n";
        }
    }
    out << '\n';

    section(out, color, "BIOS PARAMETER BLOCK");
    if (!a.bpb.looks_valid)
    {
        kv(out, "Valid", std::string("no — ") + a.bpb.invalid_reason);
    }
    else
    {
        kv(out, "Valid", "yes");
        kv(out, "Bytes/sector", std::to_string(a.bpb.bytes_per_sector));
        kv(out, "Sec/cluster", std::to_string(a.bpb.sectors_per_cluster));
        kv(out, "Reserved", std::to_string(a.bpb.reserved_sectors));
        kv(out, "FAT copies", std::to_string(a.bpb.fat_count));
        kv(out, "Root entries", std::to_string(a.bpb.root_entry_count));
        kv(out, "Total sectors", std::to_string(a.bpb.total_sectors));
        {
            std::ostringstream os;
            os << "0x" << std::hex << std::uppercase
               << static_cast<unsigned>(a.bpb.media_descriptor) << std::dec
               << "  " << media_descriptor_name(a.bpb.media_descriptor);
            kv(out, "Media", os.str());
        }
        kv(out, "Sectors/FAT", std::to_string(a.bpb.sectors_per_fat_16));
        kv(out, "Sectors/track", std::to_string(a.bpb.sectors_per_track));
        kv(out, "Heads", std::to_string(a.bpb.head_count));
        kv(out, "Hidden sectors", std::to_string(a.bpb.hidden_sectors));
        kv(out, "FAT type", fat_name(a.kind));
        kv(out, "Data clusters", std::to_string(data_cluster_count(a.bpb)));
        kv(out, "First data LBA", std::to_string(first_data_sector(a.bpb)));
        kv(out, "Root dir secs", std::to_string(root_dir_sectors(a.bpb)));
    }
    out << '\n';

    section(out, color, "VOLUME");
    if (a.ebpb.present)
    {
        std::ostringstream os;
        os << "yes (signature 0x" << std::hex << std::uppercase
           << static_cast<unsigned>(a.ebpb.boot_signature) << std::dec
           << (a.ebpb.confident ? ", FATxx FS type" : ", FS type not FATxx") << ")";
        kv(out, "Extended BPB", os.str());
        kv(out, "Drive number", std::to_string(a.ebpb.drive_number));
        kv(out, "NT flags", std::to_string(a.ebpb.nt_flags));
        kv(out, "FS type", a.ebpb.fs_type.empty() ? "(none)" : a.ebpb.fs_type);
    }
    else
    {
        kv(out, "Extended BPB",
           "no (DOS 3.x BPB; byte 0x26 is not 0x28/0x29 — serial not stored here)");
    }
    kv(out, "Serial (EBPB)",
       a.volume.serial_text.empty() ? "(none)" : a.volume.serial_text);
    kv(out, "Label (EBPB)",
       a.volume.label_ebpb.empty() ? "(none)" : a.volume.label_ebpb);
    kv(out, "Label (root)",
       a.volume.label_root.empty() ? "(none)" : a.volume.label_root);
    kv(out, "Label (best)",
       a.volume.label_best.empty() ? "(none)" : a.volume.label_best);
    out << '\n';

    if (a.bpb.looks_valid &&
        (a.kind == fat_kind::fat12 || a.kind == fat_kind::fat16))
    {
        section(out, color, "FAT");
        kv(out, "Type", fat_name(a.fat.kind));
        kv(out, "FAT bytes", std::to_string(a.fat.fat_bytes));
        kv(out, "Copies match", a.fat.copies_match ? "yes" : "NO");
        if (!a.fat.copies_match)
        {
            kv(out, "Mismatch bytes", std::to_string(a.fat.copy_mismatch_bytes));
        }
        {
            std::ostringstream os;
            os << "0x" << std::hex << std::uppercase << a.fat.fat0_media << std::dec;
            kv(out, "FAT[0]", os.str());
        }
        {
            std::ostringstream os;
            os << "0x" << std::hex << std::uppercase << a.fat.fat0_eoc << std::dec;
            kv(out, "FAT[1]", os.str());
        }
        kv(out, "Free clusters", std::to_string(a.fat.free_clusters));
        kv(out, "Allocated", std::to_string(a.fat.allocated_clusters));
        kv(out, "Bad clusters", std::to_string(a.fat.bad_clusters));
        kv(out, "EOF markers", std::to_string(a.fat.eof_markers));
        if (!a.fat.bad_list.empty())
        {
            std::ostringstream os;
            for (size_t i = 0; i < a.fat.bad_list.size(); ++i)
            {
                if (i != 0u)
                {
                    os << ", ";
                }
                os << a.fat.bad_list[i];
            }
            kv(out, "Bad list", os.str());
        }
        if (!a.fat.media_note.empty())
        {
            kv(out, "Note", a.fat.media_note);
        }
        if (!a.orphan_clusters.empty())
        {
            std::ostringstream os;
            const size_t show = std::min<size_t>(16u, a.orphan_clusters.size());
            for (size_t i = 0; i < show; ++i)
            {
                if (i != 0u)
                {
                    os << ", ";
                }
                os << a.orphan_clusters[i];
            }
            if (a.orphan_clusters.size() > show)
            {
                os << ", …";
            }
            kv(out, "Orphans", os.str());
        }
        {
            const uint32_t cluster_bytes =
                static_cast<uint32_t>(a.bpb.bytes_per_sector) * a.bpb.sectors_per_cluster;
            const uint64_t free_bytes =
                static_cast<uint64_t>(a.fat.free_clusters) * cluster_bytes;
            kv(out, "Free bytes", std::to_string(free_bytes));
        }
        out << '\n';
    }

    if (a.flux.present && a.entries.empty())
    {
        section(out, color, "DIRECTORY");
        out << "  (no FAT — booter / non-DOS volume; contents are IBM sectors above)\n\n";
    }
    else
    {
    section(out, color, "DIRECTORY");
    put_style(out, color, TUI_BOLD);
    put_style(out, color, TUI_WHITE);
    out << directory_header();
    put_style(out, color, TUI_RESET);
    out << '\n';
    size_t shown = 0;
    size_t deleted_n = 0;
    for (const dir_entry& e : a.entries)
    {
        if (e.deleted)
        {
            ++deleted_n;
            if (!opt.show_deleted)
            {
                continue;
            }
            emit_deleted_line(out, color, entry_line(e));
        }
        else
        {
            out << entry_line(e) << '\n';
        }
        ++shown;
    }
    if (a.entries.empty())
    {
        out << "  (no directory entries — not FAT, or empty/truncated root)\n";
    }
    out << "  " << shown << " entries shown, " << deleted_n << " deleted on disk\n";
    out << '\n';
    }
    } /* !cbm.present && !amiga.present && !trd.present */

    if (!a.secrets.empty())
    {
        section(out, color, "SECRETS");
        for (const std::string& s : a.secrets)
        {
            out << "  * " << s << '\n';
        }
        out << '\n';
    }

    if (opt.hex_boot && !a.cbm.present && !a.amiga.present && !a.trd.present &&
        !a.apple.present && !a.foreign.present)
    {
        section(out, color, "BOOT SECTOR HEX");
        std::span<const uint8_t> boot =
            std::span<const uint8_t>(a.image.bytes)
                .subspan(0, std::min<size_t>(512u, a.image.bytes.size()));
        if (a.flux.present && a.flux.boot.size() >= 32u)
        {
            boot = std::span<const uint8_t>(a.flux.boot);
        }
        hex_dump(out, boot);
        out << '\n';
    }
}

} /* namespace dumpfloppy */
