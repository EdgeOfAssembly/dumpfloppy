/**
 * @file bpb.cpp
 * @brief BPB / EBPB decode, including the DOS 3.3 “no serial” case.
 */
#include "dumpfloppy/bpb.hpp"
#include "dumpfloppy/util.hpp"

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace dumpfloppy
{
namespace
{

bool is_power_of_two(uint32_t v)
{
    return v != 0u && (v & (v - 1u)) == 0u;
}

bool plausible_bps(uint16_t bps)
{
    return bps == 128u || bps == 256u || bps == 512u || bps == 1024u ||
           bps == 2048u || bps == 4096u;
}

/**
 * @brief True when OEM names a DOS 3.0–3.3 volume (no serial-only EBPB).
 *
 * Compaq DOS 3.31 with a real 0x28 and OEM `IBM  3.3` is rejected on
 * purpose: prefer no serial over inventing one from a 3.x stub.
 */
bool oem_looks_dos3x(std::string_view oem)
{
    const std::string u = ascii_lower(oem);
    return u.find("3.3") != std::string::npos ||
           u.find("3.2") != std::string::npos ||
           u.find("3.1") != std::string::npos ||
           u.find("3.0") != std::string::npos;
}

/**
 * @brief True when the initial jump lands at or past the 0x29 EBPB (0x3E).
 *
 * DOS 3.3 typically uses `EB 34` (dest 0x36). DOS 4+ uses `EB 3C` (0x3E).
 */
bool jump_skips_full_ebpb(const uint8_t jump[3])
{
    if (jump[0] == 0xEBu)
    {
        const int dest = 2 + static_cast<int>(static_cast<int8_t>(jump[1]));
        return dest >= 0x3E;
    }
    if (jump[0] == 0xE9u)
    {
        const auto rel = static_cast<int16_t>(
            static_cast<uint16_t>(jump[1]) |
            (static_cast<uint16_t>(jump[2]) << 8));
        const int dest = 3 + static_cast<int>(rel);
        return dest >= 0x3E;
    }
    return false;
}

/**
 * @brief Jump + OEM look like a DOS 4+ boot that reserved space for an EBPB.
 */
bool jump_oem_looks_dos4(const bpb_info& bpb)
{
    return jump_skips_full_ebpb(bpb.jump) && is_printable_ascii(bpb.oem) &&
           !oem_looks_dos3x(bpb.oem);
}

/**
 * @brief Bytes 0x27–0x2A look like a FORMAT volume id, not a boot-code tail.
 *
 * All-zero is padding (Elvira-style). The other patterns are the IBM PC
 * loader that sits at 0x24 when 0x26 is `SUB r/m8,r8` rather than a signature.
 */
bool serial_field_plausible(std::span<const uint8_t> boot)
{
    const uint8_t a = boot[0x27];
    const uint8_t b = boot[0x28];
    const uint8_t c = boot[0x29];
    const uint8_t d = boot[0x2A];
    if (a == 0u && b == 0u && c == 0u && d == 0u)
    {
        return false;
    }
    /* MOV SS,AX; MOV SP,imm16 */
    if (a == 0x8Eu && b == 0xD0u && c == 0xBCu)
    {
        return false;
    }
    /* XOR AX,AX; MOV SS,AX */
    if (a == 0x33u && b == 0xC0u && c == 0x8Eu && d == 0xD0u)
    {
        return false;
    }
    /* ModR/M + disp16 for SUB [0x0078], r8 (IVT 1Eh diskette params). */
    if (a == 0x06u && b == 0x78u && c == 0x00u)
    {
        return false;
    }
    /* SP=7C00; PUSH SS; POP ES */
    if (a == 0x00u && b == 0x7Cu && c == 0x16u && d == 0x07u)
    {
        return false;
    }
    /* CLI; XOR AX,AX */
    if (a == 0xFAu && b == 0x33u && c == 0xC0u)
    {
        return false;
    }
    return true;
}

/**
 * @brief Gate for signature 0x28 (see parse_ebpb Doxygen).
 */
bool ebpb28_is_confident(std::span<const uint8_t> boot, const bpb_info& bpb)
{
    const bool serial_ok = serial_field_plausible(boot);
    const bool dos4 = jump_oem_looks_dos4(bpb);
    const uint8_t drive = boot[0x24];
    const bool prefix = (drive == 0x00u || drive == 0x80u) && boot[0x25] == 0u;
    const fat_kind kind = fat_kind_from_bpb(bpb);
    const bool fat12_like =
        kind == fat_kind::fat12 || kind == fat_kind::unknown;
    return serial_ok && (dos4 || (prefix && !fat12_like));
}

} /* namespace */

bpb_info parse_bpb(std::span<const uint8_t> boot)
{
    bpb_info b{};
    if (boot.size() < 36u)
    {
        b.invalid_reason = "boot sector shorter than 36 bytes";
        return b;
    }

    b.jump[0] = boot[0];
    b.jump[1] = boot[1];
    b.jump[2] = boot[2];
    b.oem.assign(reinterpret_cast<const char*>(boot.data() + 3), 8);

    b.bytes_per_sector = read_le16(boot, 11);
    b.sectors_per_cluster = boot[13];
    b.reserved_sectors = read_le16(boot, 14);
    b.fat_count = boot[16];
    b.root_entry_count = read_le16(boot, 17);
    b.total_sectors_16 = read_le16(boot, 19);
    b.media_descriptor = boot[21];
    b.sectors_per_fat_16 = read_le16(boot, 22);
    b.sectors_per_track = read_le16(boot, 24);
    b.head_count = read_le16(boot, 26);
    b.hidden_sectors = read_le32(boot, 28);
    b.total_sectors_32 = (boot.size() >= 36u) ? read_le32(boot, 32) : 0u;
    b.total_sectors = (b.total_sectors_16 != 0u) ? b.total_sectors_16 : b.total_sectors_32;

    if (!plausible_bps(b.bytes_per_sector))
    {
        b.invalid_reason = "bytes/sector is not a FAT sector size";
        return b;
    }
    if (!is_power_of_two(b.sectors_per_cluster) || b.sectors_per_cluster > 128u)
    {
        b.invalid_reason = "sectors/cluster is not a FAT cluster size";
        return b;
    }
    if (b.reserved_sectors == 0u)
    {
        b.invalid_reason = "reserved sector count is 0";
        return b;
    }
    if (b.fat_count == 0u)
    {
        b.invalid_reason = "FAT count is 0";
        return b;
    }
    if (b.total_sectors == 0u)
    {
        b.invalid_reason = "total sector count is 0";
        return b;
    }
    if (b.sectors_per_fat_16 == 0u)
    {
        b.invalid_reason = "sectors/FAT is 0 (FAT32 or not FAT)";
        return b;
    }
    b.looks_valid = true;
    return b;
}

ebpb_info parse_ebpb(std::span<const uint8_t> boot, const bpb_info& bpb)
{
    ebpb_info e{};
    if (boot.size() < 0x3Eu)
    {
        return e;
    }

    const uint8_t sig = boot[0x26];
    if (sig != k_ebpb_sig_28 && sig != k_ebpb_sig_29)
    {
        return e;
    }

    if (sig == k_ebpb_sig_28 && !ebpb28_is_confident(boot, bpb))
    {
        /* 0x28 collided with DOS 3.x `SUB r/m8,r8` — no invented serial. */
        return e;
    }

    e.present = true;
    e.boot_signature = sig;
    e.drive_number = boot[0x24];
    e.nt_flags = boot[0x25];
    e.volume_serial = read_le32(boot, 0x27);
    e.has_serial = true;

    if (sig == k_ebpb_sig_29)
    {
        e.volume_label = ascii_field(boot, 0x2B, 11);
        e.has_label = !e.volume_label.empty();
        e.fs_type.assign(reinterpret_cast<const char*>(boot.data() + 0x36), 8);
        while (!e.fs_type.empty() && e.fs_type.back() == ' ')
        {
            e.fs_type.pop_back();
        }
        const std::string fs_up = ascii_lower(e.fs_type);
        e.confident = fs_up.starts_with("fat");
        if (!e.confident && !is_printable_ascii(e.fs_type))
        {
            /* 0x29 collided with boot code — do not trust serial/label. */
            e.present = false;
            e.has_serial = false;
            e.has_label = false;
            e.confident = false;
            e.volume_serial = 0;
            e.volume_label.clear();
            e.fs_type.clear();
            return e;
        }
    }
    else
    {
        e.confident = true;
    }
    return e;
}

uint32_t root_dir_sectors(const bpb_info& bpb)
{
    if (bpb.bytes_per_sector == 0u)
    {
        return 0;
    }
    const uint32_t bytes = static_cast<uint32_t>(bpb.root_entry_count) * 32u;
    return (bytes + bpb.bytes_per_sector - 1u) / bpb.bytes_per_sector;
}

uint32_t first_data_sector(const bpb_info& bpb)
{
    return static_cast<uint32_t>(bpb.reserved_sectors) +
           static_cast<uint32_t>(bpb.fat_count) * bpb.sectors_per_fat_16 +
           root_dir_sectors(bpb);
}

uint32_t data_cluster_count(const bpb_info& bpb)
{
    const uint32_t first = first_data_sector(bpb);
    if (bpb.total_sectors <= first || bpb.sectors_per_cluster == 0u)
    {
        return 0;
    }
    return (bpb.total_sectors - first) / bpb.sectors_per_cluster;
}

fat_kind fat_kind_from_bpb(const bpb_info& bpb)
{
    if (!bpb.looks_valid)
    {
        return fat_kind::unknown;
    }
    const uint32_t n = data_cluster_count(bpb);
    if (n < 4085u)
    {
        return fat_kind::fat12;
    }
    if (n < 65525u)
    {
        return fat_kind::fat16;
    }
    return fat_kind::fat32;
}

std::string describe_jump(const uint8_t jump[3])
{
    char buf[80] = {};
    if (jump[0] == 0xEBu)
    {
        const unsigned rel = jump[1];
        std::snprintf(buf, sizeof(buf), "EB %02X %02X  (short jmp +0x%02X)",
                      jump[1], jump[2], rel);
        return buf;
    }
    if (jump[0] == 0xE9u)
    {
        const unsigned rel = static_cast<unsigned>(jump[1]) |
                             (static_cast<unsigned>(jump[2]) << 8);
        std::snprintf(buf, sizeof(buf), "E9 %02X %02X  (near jmp +0x%04X)",
                      jump[1], jump[2], rel);
        return buf;
    }
    if (jump[0] == 0x90u && jump[1] == 0x90u)
    {
        return "90 90 …  (NOP sled; unusual boot)";
    }
    std::snprintf(buf, sizeof(buf), "%02X %02X %02X  (not a DOS jump)",
                  jump[0], jump[1], jump[2]);
    return buf;
}

} /* namespace dumpfloppy */
