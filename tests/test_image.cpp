/**
 * @file test_image.cpp
 * @brief End-to-end analysis of synthetic FAT12 and booter images.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/util.hpp"
#include "image_builder.hpp"

#include <tui/ansi.h>

#include <catch2/catch_test_macros.hpp>
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
    REQUIRE(a.volume.serial_text == "1234-ABCD");
    REQUIRE(a.volume.label_ebpb == "TESTVOL");
    REQUIRE_FALSE(a.volume.label_root.empty());
    REQUIRE(a.boot.has_aa55);
    REQUIRE(a.boot.has_jump);
    REQUIRE_FALSE(a.boot.is_booter);
    REQUIRE(a.boot.kind == dumpfloppy::boot_class::dos_non_system);

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
    REQUIRE(a.volume.serial_text.empty());
    REQUIRE(a.volume.label_root.find("TESTVOL") != std::string::npos);
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

TEST_CASE("deleted entries blink on light-red when colour is on", "[image][ansi]")
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
    REQUIRE(s.find(TUI_BLINK) != std::string::npos);
    REQUIRE(s.find(TUI_BOLD) != std::string::npos);
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
    REQUIRE(s.find("Checksum") != std::string::npos);
    REQUIRE(s.find("RHSVDA") == std::string::npos);
    REQUIRE(s.find("cl=") == std::string::npos);
    REQUIRE(s.find("\\HELLO") == std::string::npos);
    REQUIRE(s.find("HELLO.TXT") != std::string::npos);
    REQUIRE(s.find("ec252e95cb88a8cb5c9682cd892a0ccf") != std::string::npos);

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
