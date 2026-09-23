/**
 * @file test_image.cpp
 * @brief End-to-end analysis of synthetic FAT12 and booter images.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/util.hpp"
#include "dumpfloppy/volume.hpp"
#include "image_builder.hpp"

#include <tui/ansi.h>

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace
{

std::filesystem::path write_temp(const std::vector<uint8_t>& bytes,
                                 const std::string& name)
{
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto path = dir / name;
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    REQUIRE(out);
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    REQUIRE(out);
    return path;
}

} /* namespace */

TEST_CASE("FAT12 sample exposes serial, both labels, and deleted file", "[image]")
{
    const auto bytes = dumpfloppy_test::make_fat12_sample(0x1234ABCDu, true);
    const auto path = write_temp(bytes, "sample.ima");
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded);
    REQUIRE(loaded->container == dumpfloppy::container_kind::ima_winimage);

    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.bpb.looks_valid);
    REQUIRE(a.kind == dumpfloppy::fat_kind::fat12);
    REQUIRE(a.ebpb.present);
    REQUIRE(a.ebpb.confident);
    REQUIRE(a.ebpb.has_serial);
    REQUIRE(a.ebpb.boot_signature == dumpfloppy::k_ebpb_sig_29);
    REQUIRE(a.volume.serial_text == "1234-ABCD");
    REQUIRE(a.volume.label_ebpb == "TESTVOL");
    REQUIRE_FALSE(a.volume.label_root.empty());
    REQUIRE(a.boot.has_aa55);
    REQUIRE(a.boot.has_jump);
    REQUIRE_FALSE(a.boot.is_booter);
    REQUIRE(a.boot.kind == dumpfloppy::boot_class::dos_non_system);
    REQUIRE_FALSE(a.cbm.present);

    bool saw_hello = false;
    bool saw_deleted = false;
    for (const dumpfloppy::dir_entry& e : a.entries)
    {
        if (e.name_83 == "HELLO.TXT" && !e.deleted)
        {
            saw_hello = true;
            REQUIRE(e.size == 14);
            REQUIRE(e.first_cluster == 2);
        }
        if (e.deleted)
        {
            saw_deleted = true;
            REQUIRE(e.name_83.find('?') == 0);
        }
    }
    REQUIRE(saw_hello);
    REQUIRE(saw_deleted);
}

TEST_CASE("DOS 3.3 BPB does not invent a volume serial", "[image]")
{
    const auto bytes = dumpfloppy_test::make_fat12_sample(0xDEADBEEFu, false);
    const auto path = write_temp(bytes, "dos33.img");
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE_FALSE(a.ebpb.present);
    REQUIRE_FALSE(a.ebpb.has_serial);
    REQUIRE_FALSE(a.ebpb.confident);
    REQUIRE(a.volume.serial_text.empty());
    REQUIRE(a.volume.label_root.find("TESTVOL") != std::string::npos);
}

TEST_CASE("DOS 3.3 boot opcode 0x28 is not an EBPB serial", "[image][ebpb]")
{
    /* Classic PC-DOS 3.3: JMP to 0x36, OEM MSDOS3.3, stub at 0x24.
       0x28 is SUB r/m8,r8 (SUB [0x0078], AL) — not a signature. */
    auto bytes = dumpfloppy_test::make_fat12_sample(0xDEADBEEFu, false);
    bytes[0] = 0xEB;
    bytes[1] = 0x34;
    bytes[2] = 0x90;
    std::memcpy(bytes.data() + 3, "MSDOS3.3", 8);
    bytes[0x24] = 0xFA; /* CLI */
    bytes[0x25] = 0x33; /* XOR … (modrm follows at 0x26 in some stubs) */
    bytes[0x26] = 0x28; /* SUB r/m8, r8 */
    bytes[0x27] = 0x06;
    bytes[0x28] = 0x78;
    bytes[0x29] = 0x00;
    bytes[0x2A] = 0x8E;

    const auto path = write_temp(bytes, "dos33-sub28.img");
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.bpb.looks_valid);
    REQUIRE(a.kind == dumpfloppy::fat_kind::fat12);
    REQUIRE_FALSE(a.ebpb.present);
    REQUIRE_FALSE(a.ebpb.has_serial);
    REQUIRE_FALSE(a.ebpb.confident);
    REQUIRE(a.volume.serial_text.empty());
    REQUIRE(a.volume.label_root.find("TESTVOL") != std::string::npos);
}

TEST_CASE("DOS 3.3 OEM with 0x28 in BPB padding is not a serial", "[image][ebpb]")
{
    /* Elvira-style: EB 34 / MSDOS3.3, zeros then a planted 0x28 + 1234-ABCD.
       Drive/NT look like an EBPB prefix; OEM still says 3.3. */
    auto bytes = dumpfloppy_test::make_fat12_sample(0x1234ABCDu, false);
    bytes[0] = 0xEB;
    bytes[1] = 0x34;
    bytes[2] = 0x90;
    std::memcpy(bytes.data() + 3, "MSDOS3.3", 8);
    bytes[0x24] = 0x00;
    bytes[0x25] = 0x00;
    bytes[0x26] = 0x28;
    bytes[0x27] = 0xCD;
    bytes[0x28] = 0xAB;
    bytes[0x29] = 0x34;
    bytes[0x2A] = 0x12;

    const auto path = write_temp(bytes, "dos33-pad28.img");
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE_FALSE(a.ebpb.present);
    REQUIRE_FALSE(a.ebpb.has_serial);
    REQUIRE_FALSE(a.ebpb.confident);
    REQUIRE(a.volume.serial_text.empty());
}

TEST_CASE("DOS 4 serial-only EBPB 0x28 still yields a serial", "[image][ebpb]")
{
    auto bytes = dumpfloppy_test::make_fat12_sample(0x1234ABCDu, false);
    std::memcpy(bytes.data() + 3, "MSDOS4.0", 8);
    bytes[0x24] = 0x00;
    bytes[0x25] = 0x00;
    bytes[0x26] = 0x28;
    bytes[0x27] = 0xCD;
    bytes[0x28] = 0xAB;
    bytes[0x29] = 0x34;
    bytes[0x2A] = 0x12;

    const auto path = write_temp(bytes, "dos4-ebpb28.img");
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.ebpb.present);
    REQUIRE(a.ebpb.confident);
    REQUIRE(a.ebpb.has_serial);
    REQUIRE(a.ebpb.boot_signature == dumpfloppy::k_ebpb_sig_28);
    REQUIRE(a.volume.serial_text == "1234-ABCD");
    REQUIRE(a.volume.label_ebpb.empty());
}

TEST_CASE("custom booter is classified as a booter disk", "[image]")
{
    const auto bytes = dumpfloppy_test::make_booter_sample();
    const auto path = write_temp(bytes, "game.img");
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.boot.is_booter);
    REQUIRE(a.boot.kind == dumpfloppy::boot_class::custom_booter);
    REQUIRE_FALSE(a.bpb.looks_valid);
}

TEST_CASE("deleted entries use light-red and bold white when colour is on", "[image][ansi]")
{
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    const auto path = write_temp(bytes, "color.ima");
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));

    dumpfloppy::report_options opt{};
    opt.color = true;
    opt.hex_boot = false;
    std::ostringstream colored;
    dumpfloppy::write_report(a, colored, opt);
    const std::string s = colored.str();
    REQUIRE(s.find(TUI_BG_BRIGHT_RED) != std::string::npos);
    REQUIRE(s.find(TUI_WHITE) != std::string::npos);
    REQUIRE(s.find(TUI_BOLD) != std::string::npos);
    REQUIRE(s.find(TUI_BLINK) == std::string::npos);
    REQUIRE(s.find("1234-ABCD") != std::string::npos);

    opt.color = false;
    std::ostringstream plain;
    dumpfloppy::write_report(a, plain, opt);
    REQUIRE(plain.str().find("\x1b[") == std::string::npos);
    REQUIRE(plain.str().find("deleted") != std::string::npos);
}

TEST_CASE("directory table is 8.3 names, left-justified, no cl= prefix", "[image]")
{
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    const auto path = write_temp(bytes, "listing.ima");
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    dumpfloppy::report_options opt{};
    opt.color = false;
    opt.hex_boot = false;
    std::ostringstream plain;
    dumpfloppy::write_report(a, plain, opt);
    const std::string s = plain.str();

    REQUIRE(s.find("Name") != std::string::npos);
    REQUIRE(s.find("Attributes") != std::string::npos);
    REQUIRE(s.find("Cluster") != std::string::npos);
    REQUIRE(s.find("Modified") != std::string::npos);
    REQUIRE(s.find("Type") != std::string::npos);
    REQUIRE(s.find("XXH64 Checksum") != std::string::npos);
    REQUIRE(s.find("RHSVDA") == std::string::npos);
    REQUIRE(s.find("cl=") == std::string::npos);
    REQUIRE(s.find("\\HELLO") == std::string::npos);
    REQUIRE(s.find("HELLO.TXT") != std::string::npos);
    REQUIRE(s.find("a41fb567443800ac") != std::string::npos);

    std::string hello_line;
    std::istringstream in(s);
    std::string line;
    while (std::getline(in, line))
    {
        if (line.find("HELLO.TXT") != std::string::npos)
        {
            hello_line = line;
            break;
        }
    }
    REQUIRE_FALSE(hello_line.empty());
    REQUIRE(hello_line.find('\\') == std::string::npos);
    /* indent 2 + mark 1 + 2-space gap. */
    REQUIRE(hello_line.substr(5, 9) == "HELLO.TXT");

    std::string gone_line;
    std::istringstream in2(s);
    while (std::getline(in2, line))
    {
        if (line.find("?ONE.TXT") != std::string::npos)
        {
            gone_line = line;
            break;
        }
    }
    REQUIRE_FALSE(gone_line.empty());
    REQUIRE(gone_line.find("  deleted") == std::string::npos);
    REQUIRE(s.find("volume label") == std::string::npos);
}

TEST_CASE("volume label is 11-char text without a fake 8.3 dot", "[image]")
{
    const uint8_t raw[11] = {' ', ' ', 'B', 'A', 'T', 'M', 'A', 'N', ' ', '#', '1'};
    REQUIRE(dumpfloppy::format_volume_label(raw, false) == "BATMAN #1");
}

TEST_CASE("format_volume_serial is high-word first", "[util]")
{
    REQUIRE(dumpfloppy::format_volume_serial(0x1234ABCDu) == "1234-ABCD");
    REQUIRE(dumpfloppy::format_volume_serial(0u) == "0000-0000");
}

TEST_CASE("load_image rejects missing files", "[image]")
{
    auto loaded = dumpfloppy::load_image("/no/such/dumpfloppy-missing.ima");
    REQUIRE_FALSE(loaded);
    REQUIRE(loaded.error().find("no such") != std::string::npos);
}

TEST_CASE("86F flux with empty CHS does not FAT-walk the bitstream",
          "[image][86f][flux]")
{
    /* 86BF magic so inspect_86f sets present; BPB at offset 11 stays a
       valid FAT12 so a missing CHS gate would list HELLO.TXT from raw bytes. */
    auto bytes = dumpfloppy_test::make_fat12_sample();
    REQUIRE(bytes.size() >= 8u);
    bytes[0] = '8';
    bytes[1] = '6';
    bytes[2] = 'B';
    bytes[3] = 'F';
    bytes[4] = 12;
    bytes[5] = 2;
    bytes[6] = 0;
    bytes[7] = 0;

    dumpfloppy::floppy_image img{};
    img.bytes = std::move(bytes);
    img.path = "synthetic.86f";
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(img));
    REQUIRE(a.flux.present);
    REQUIRE(a.flux.format_name == "86BOX 86F");
    REQUIRE(a.flux.assembled_chs.empty());
    REQUIRE(a.entries.empty());

    bool saw_hello = false;
    for (const dumpfloppy::dir_entry& e : a.entries)
    {
        if (e.name_83.find("HELLO") != std::string::npos)
        {
            saw_hello = true;
        }
    }
    REQUIRE_FALSE(saw_hello);

    bool saw_chs = false;
    for (const std::string& s : a.secrets)
    {
        if (s.find("CHS") != std::string::npos)
        {
            saw_chs = true;
        }
    }
    REQUIRE(saw_chs);
}

TEST_CASE("volume_bytes prefers assembled_chs over image.bytes", "[volume]")
{
    dumpfloppy::analysis a{};
    a.image.bytes = {1, 2, 3};

    REQUIRE(dumpfloppy::volume_bytes(a).data() == a.image.bytes.data());
    REQUIRE(dumpfloppy::volume_bytes(a).size() == 3u);
    REQUIRE(&dumpfloppy::volume_bytes_mut(a) == &a.image.bytes);

    dumpfloppy::sector_store raw = dumpfloppy::make_sector_store(a);
    REQUIRE(raw.bytes.size() == 3u);
    REQUIRE(raw.sector_size == dumpfloppy::k_ibm_sector_bytes);

    const dumpfloppy::sector_store ibm_raw = dumpfloppy::make_ibm_store(
        a.flux.assembled_chs, a.image.bytes, a.bpb.bytes_per_sector);
    REQUIRE(ibm_raw.bytes.data() == raw.bytes.data());
    REQUIRE(ibm_raw.sector_size == raw.sector_size);

    a.bpb.bytes_per_sector = 1024;
    raw = dumpfloppy::make_sector_store(a);
    REQUIRE(raw.sector_size == 1024u);
    REQUIRE(dumpfloppy::make_ibm_store(a.flux.assembled_chs, a.image.bytes,
                                       a.bpb.bytes_per_sector)
                .sector_size == 1024u);

    a.flux.assembled_chs = {9, 9};
    REQUIRE(dumpfloppy::volume_bytes(a).data() == a.flux.assembled_chs.data());
    REQUIRE(dumpfloppy::volume_bytes(a).size() == 2u);
    REQUIRE(&dumpfloppy::volume_bytes_mut(a) == &a.flux.assembled_chs);
    REQUIRE(&dumpfloppy::ibm_volume_mut(a.flux.assembled_chs, a.image.bytes) ==
            &a.flux.assembled_chs);

    const dumpfloppy::sector_store chs = dumpfloppy::make_sector_store(a);
    REQUIRE(chs.sector_size == dumpfloppy::k_ibm_sector_bytes);
    REQUIRE(chs.bytes.size() == 2u);
    REQUIRE(dumpfloppy::make_ibm_store(a.flux.assembled_chs, a.image.bytes,
                                       a.bpb.bytes_per_sector)
                .sector_size == dumpfloppy::k_ibm_sector_bytes);

    dumpfloppy::volume_bytes_mut(a)[0] = 7;
    REQUIRE(a.flux.assembled_chs[0] == 7);
    REQUIRE(a.image.bytes[0] == 1);
}
