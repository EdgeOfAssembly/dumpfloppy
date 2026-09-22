/**
 * @file report.cpp
 * @brief Colourful floppy secret dump. Deleted names use TUI light-red + blink.
 */
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/bpb.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/geometry.hpp"
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
        default:
            return "raw (extension is not .img/.ima)";
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
        /* Light-red background, white bold blinking text (user request). */
        out << TUI_BG_BRIGHT_RED << TUI_WHITE << TUI_BOLD << TUI_BLINK << line
            << TUI_RESET << '\n';
    }
    else
    {
        out << line << '\n';
    }
}

/**
 * Left-justify @p s into exactly @p width characters (pad or clip).
 * FAT12 8.3 names are at most 12 characters (`NAME.EXT`); the Name
 * column is 15 so they always fit.
 */
std::string field(std::string_view s, size_t width)
{
    std::string out;
    out.reserve(width);
    const size_t n = std::min(s.size(), width);
    out.append(s.data(), n);
    if (n < width)
    {
        out.append(width - n, ' ');
    }
    return out;
}

/* Fixed directory-table widths (all columns left-justified). */
constexpr size_t k_w_mark = 1;
constexpr size_t k_w_name = 15;      /* 8.3 + slack */
constexpr size_t k_w_attr = 10;      /* header "Attributes" */
constexpr size_t k_w_size = 10;      /* FAT size is 32-bit */
constexpr size_t k_w_cluster = 7;    /* header "Cluster"; FAT12 max 4084 */
constexpr size_t k_w_modified = 19;  /* YYYY-MM-DD HH:MM:SS */

std::string directory_header()
{
    std::ostringstream os;
    os << "  " << field(" ", k_w_mark) << ' ' << field("Name", k_w_name) << "  "
       << field("Attributes", k_w_attr) << "  " << field("Size", k_w_size) << "  "
       << field("Cluster", k_w_cluster) << "  " << field("Modified", k_w_modified);
    return os.str();
}

std::string entry_line(const dir_entry& e)
{
    const char mark = e.deleted ? 'D' : ' ';
    const std::string modified =
        format_dos_date(e.write_date) + " " + format_dos_time(e.write_time);
    std::ostringstream os;
    os << "  " << field(std::string_view(&mark, 1), k_w_mark) << ' '
       << field(e.name_83, k_w_name) << "  " << field(format_attributes(e.attributes), k_w_attr)
       << "  " << field(std::to_string(e.size), k_w_size) << "  "
       << field(std::to_string(e.first_cluster), k_w_cluster) << "  "
       << field(modified, k_w_modified);
    if (!e.magic.empty())
    {
        os << "  [" << e.magic << "]";
    }
    if (!e.notes.empty())
    {
        os << "  " << e.notes;
    }
    if (e.deleted)
    {
        os << "  deleted";
    }
    if (e.after_terminator)
    {
        os << "  after-0x00";
    }
    return os.str();
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
    kv(out, "Container", container_name(a.image.container));
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

    section(out, color, "BOOT");
    kv(out, "Jump", describe_jump(a.bpb.jump));
    kv(out, "OEM", a.bpb.oem.empty() ? "(none)" : a.bpb.oem);
    kv(out, "55 AA", a.boot.has_aa55 ? "yes (offset 510)" : "no");
    kv(out, "Booter disk", a.boot.is_booter ? "YES (custom/game boot payload)" : "no");
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

    section(out, color, "DIRECTORY");
    out << directory_header() << '\n';
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

    if (!a.secrets.empty())
    {
        section(out, color, "SECRETS");
        for (const std::string& s : a.secrets)
        {
            out << "  * " << s << '\n';
        }
        out << '\n';
    }

    if (opt.hex_boot)
    {
        section(out, color, "BOOT SECTOR HEX");
        const std::span<const uint8_t> boot =
            std::span<const uint8_t>(a.image.bytes)
                .subspan(0, std::min<size_t>(512u, a.image.bytes.size()));
        hex_dump(out, boot);
        out << '\n';
    }
}

} /* namespace dumpfloppy */
