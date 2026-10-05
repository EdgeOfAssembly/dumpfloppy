/**
 * @file test_ws040.cpp
 * @brief DOS 1.x FAT, ImageDisk, mov-prologue boot class, and CP/M names.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/fat12_codec.h"
#include "dumpfloppy/foreign.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/report.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

std::filesystem::path write_temp(const std::vector<uint8_t>& bytes, const std::string& name)
{
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-ws040";
    std::filesystem::create_directories(dir);
    const auto path = dir / name;
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    REQUIRE(out);
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    REQUIRE(out);
    return path;
}

dumpfloppy::analysis analyse_bytes(const std::vector<uint8_t>& bytes, const std::string& name)
{
    const auto path = write_temp(bytes, name);
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded);
    return dumpfloppy::analyse(std::move(*loaded));
}

void put_eoc(std::vector<uint8_t>& img, std::size_t fat_off, uint32_t cluster)
{
    REQUIRE(fat12_entry_set(img.data() + fat_off, 512u, cluster, 0x0FFFu) == 0);
}

void put_dirent(std::vector<uint8_t>& img, std::size_t off, const char name[11],
                uint16_t cluster, uint32_t size)
{
    std::memcpy(img.data() + off, name, 11u);
    img[off + 11u] = 0x20u;
    img[off + 26u] = static_cast<uint8_t>(cluster & 0xFFu);
    img[off + 27u] = static_cast<uint8_t>((cluster >> 8) & 0xFFu);
    img[off + 28u] = static_cast<uint8_t>(size & 0xFFu);
    img[off + 29u] = static_cast<uint8_t>((size >> 8) & 0xFFu);
    img[off + 30u] = static_cast<uint8_t>((size >> 16) & 0xFFu);
    img[off + 31u] = static_cast<uint8_t>((size >> 24) & 0xFFu);
}

std::vector<uint8_t> dos1_160(const char* payload5)
{
    std::vector<uint8_t> img(163840u, 0u);
    img[512] = 0xFEu;
    img[513] = 0xFFu;
    img[514] = 0xFFu;
    img[1024] = 0xFEu;
    img[1025] = 0xFFu;
    img[1026] = 0xFFu;
    put_eoc(img, 512u, 2u);
    put_eoc(img, 1024u, 2u);
    put_dirent(img, 1536u, "HELLO   TXT", 2u, 5u);
    std::memcpy(img.data() + (7u * 512u), payload5, 5u);
    return img;
}

const dumpfloppy::dir_entry* find_name(const dumpfloppy::analysis& a, const char* name)
{
    for (const dumpfloppy::dir_entry& e : a.entries)
    {
        if (e.name_83 == name && !e.deleted)
        {
            return &e;
        }
    }
    return nullptr;
}

std::string report_text(const dumpfloppy::analysis& a)
{
    dumpfloppy::report_options opt;
    opt.color = false;
    std::ostringstream os;
    dumpfloppy::write_report(a, os, opt);
    return os.str();
}

} /* namespace */

TEST_CASE("DOS 1.x 160K FAT without a BPB lists and extracts", "[ws040][dos1]")
{
    const char payload[5] = {'H', 'E', 'L', 'L', 'O'};
    const dumpfloppy::analysis a = analyse_bytes(dos1_160(payload), "dos1-160.img");
    REQUIRE(a.bpb.looks_valid);
    REQUIRE(a.dos1);
    REQUIRE(a.kind == dumpfloppy::fat_kind::fat12);
    REQUIRE(a.bpb.sectors_per_cluster == 1u);
    REQUIRE(report_text(a).find("DOS 1.x / FAT12") != std::string::npos);
    REQUIRE(a.bpb.media_descriptor == 0xFEu);
    const dumpfloppy::dir_entry* hello = find_name(a, "HELLO.TXT");
    REQUIRE(hello != nullptr);
    REQUIRE(hello->size == 5u);
    REQUIRE(hello->first_cluster == 2u);

    const auto dest = std::filesystem::temp_directory_path() / "dumpfloppy-ws040-x160";
    std::filesystem::remove_all(dest);
    dumpfloppy::extract_options ex;
    ex.enabled = true;
    ex.dest_dir = dest;
    ex.patterns = {"HELLO.TXT"};
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, ex, err) == 1);
    std::ifstream in(dest / "HELLO.TXT", std::ios::binary);
    REQUIRE(in);
    std::string got(5, '\0');
    in.read(got.data(), 5);
    REQUIRE(in.gcount() == 5);
    REQUIRE(got == "HELLO");
}

TEST_CASE("DOS 1.x 320K FAT uses 1024-byte clusters", "[ws040][dos1]")
{
    std::vector<uint8_t> img(327680u, 0u);
    img[512] = 0xFFu;
    img[513] = 0xFFu;
    img[514] = 0xFFu;
    img[1024] = 0xFFu;
    img[1025] = 0xFFu;
    img[1026] = 0xFFu;
    put_eoc(img, 512u, 2u);
    put_eoc(img, 1024u, 2u);
    put_eoc(img, 512u, 23u);
    put_eoc(img, 1024u, 23u);
    put_dirent(img, 1536u, "FILE1   BIN", 2u, 5u);
    put_dirent(img, 1568u, "FILE2   BIN", 23u, 4u);
    const char mark[4] = {'A', 'B', 'C', 'D'};
    const std::size_t file2_off = (7u + (23u - 2u) * 2u) * 512u;
    std::memcpy(img.data() + file2_off, mark, 4u);

    const dumpfloppy::analysis a = analyse_bytes(img, "dos1-320.img");
    REQUIRE(a.bpb.looks_valid);
    REQUIRE(a.bpb.bytes_per_sector == 512u);
    REQUIRE(a.bpb.sectors_per_cluster == 2u);
    const dumpfloppy::dir_entry* file2 = find_name(a, "FILE2.BIN");
    REQUIRE(file2 != nullptr);
    REQUIRE(file2->first_cluster == 23u);

    const auto dest = std::filesystem::temp_directory_path() / "dumpfloppy-ws040-x320";
    std::filesystem::remove_all(dest);
    dumpfloppy::extract_options ex;
    ex.enabled = true;
    ex.dest_dir = dest;
    ex.patterns = {"FILE2.BIN"};
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, ex, err) == 1);
    std::ifstream in(dest / "FILE2.BIN", std::ios::binary);
    REQUIRE(in);
    std::string got(4, '\0');
    in.read(got.data(), 4);
    REQUIRE(got == "ABCD");
}

TEST_CASE("a short image with a DOS 1.x FAT signature stays invalid", "[ws040][dos1]")
{
    std::vector<uint8_t> img(4096u, 0u);
    img[512] = 0xFEu;
    img[513] = 0xFFu;
    img[514] = 0xFFu;
    img[1536] = static_cast<uint8_t>('A');
    const dumpfloppy::analysis a = analyse_bytes(img, "dos1-short.img");
    REQUIRE_FALSE(a.bpb.looks_valid);
}

TEST_CASE("ImageDisk sector numbers assemble in numeric order", "[ws040][imd]")
{
    std::vector<uint8_t> imd;
    const std::string hdr = "IMD 1.18: t\r\n";
    imd.insert(imd.end(), hdr.begin(), hdr.end());
    imd.push_back(0x1Au);
    imd.push_back(5u);
    imd.push_back(0u);
    imd.push_back(0u);
    imd.push_back(2u);
    imd.push_back(2u);
    imd.push_back(2u);
    imd.push_back(1u);
    imd.push_back(2u);
    imd.push_back(0x22u);
    imd.push_back(2u);
    imd.push_back(0x11u);

    const dumpfloppy::imd_image dec = dumpfloppy::decode_imd(imd);
    REQUIRE(dec.magic);
    REQUIRE(dec.sectors.size() == 1024u);
    REQUIRE(dec.sectors[0] == 0x11u);
    REQUIRE(dec.sectors[511] == 0x11u);
    REQUIRE(dec.sectors[512] == 0x22u);
    REQUIRE(dec.sectors[1023] == 0x22u);

    const dumpfloppy::analysis a = analyse_bytes(imd, "two.imd");
    REQUIRE(a.imd);
    REQUIRE(a.image.bytes == dec.sectors);
    REQUIRE(report_text(a).find("49 4D 44") == std::string::npos);
}

TEST_CASE("ImageDisk wrapping a 160K FAT lists HELLO.TXT", "[ws040][imd]")
{
    const std::vector<uint8_t> vol = dos1_160("HELLO");
    std::vector<uint8_t> imd;
    const std::string hdr = "IMD 1.18: synthetic\r\n";
    imd.insert(imd.end(), hdr.begin(), hdr.end());
    imd.push_back(0x1Au);
    for (unsigned cyl = 0; cyl < 40u; ++cyl)
    {
        imd.push_back(5u);
        imd.push_back(static_cast<uint8_t>(cyl));
        imd.push_back(0u);
        imd.push_back(8u);
        imd.push_back(2u);
        for (unsigned s = 1; s <= 8u; ++s)
        {
            imd.push_back(static_cast<uint8_t>(s));
        }
        for (unsigned s = 0; s < 8u; ++s)
        {
            const std::size_t off = (static_cast<std::size_t>(cyl) * 8u + s) * 512u;
            imd.push_back(1u);
            imd.insert(imd.end(), vol.begin() + static_cast<std::ptrdiff_t>(off),
                       vol.begin() + static_cast<std::ptrdiff_t>(off + 512u));
        }
    }
    const dumpfloppy::analysis a = analyse_bytes(imd, "hello.imd");
    REQUIRE(a.imd);
    REQUIRE(a.bpb.looks_valid);
    REQUIRE(find_name(a, "HELLO.TXT") != nullptr);
    const std::string text = report_text(a);
    REQUIRE(text.find("ImageDisk / FAT12") != std::string::npos);
    REQUIRE(text.find("1.18") == std::string::npos);
    REQUIRE(text.find("49 4D 44") == std::string::npos);
}

TEST_CASE("ImageDisk with no sector payload is an empty ImageDisk image", "[ws040][imd]")
{
    const std::vector<uint8_t> imd{'I', 'M', 'D', ' ', 0x1A};
    const dumpfloppy::analysis a = analyse_bytes(imd, "empty.imd");
    REQUIRE(a.imd);
    REQUIRE(a.entries.empty());
    const std::string text = report_text(a);
    REQUIRE(text.find("ImageDisk") != std::string::npos);
    const auto jump = text.find("Jump");
    REQUIRE(jump != std::string::npos);
    const auto line_end = text.find('\n', jump);
    REQUIRE(text.substr(jump, line_end - jump).find("49 4D 44") == std::string::npos);
}

TEST_CASE("mov ax,0 prologue is a custom booter", "[ws040][boot]")
{
    std::vector<uint8_t> mov(512u, 0u);
    mov[0] = 0xB8u;
    mov[3] = 0x8Eu;
    mov[4] = 0xD8u;
    const dumpfloppy::analysis ds = analyse_bytes(mov, "mov-ds.bin");
    REQUIRE(ds.boot.kind == dumpfloppy::boot_class::custom_booter);
    REQUIRE(ds.boot.is_booter);
    REQUIRE(ds.boot.kind_text.find("mov prologue") != std::string::npos);

    mov[4] = 0xD0u;
    const dumpfloppy::analysis ss = analyse_bytes(mov, "mov-ss.bin");
    REQUIRE(ss.boot.kind == dumpfloppy::boot_class::custom_booter);
    REQUIRE(ss.boot.is_booter);

    const std::vector<uint8_t> zeros(512u, 0u);
    const dumpfloppy::analysis z = analyse_bytes(zeros, "zeros.bin");
    REQUIRE(z.boot.kind == dumpfloppy::boot_class::not_bootable);
    REQUIRE_FALSE(z.boot.is_booter);
}

TEST_CASE("CP/M directory lists WS.COM and keeps a deleted slot deleted", "[ws040][cpm]")
{
    std::vector<uint8_t> img(143360u, 0xE5u);
    auto put = [&](std::size_t off, uint8_t user, const char* name11, uint8_t extent)
    {
        img[off] = user;
        std::memcpy(img.data() + off + 1u, name11, 11u);
        img[off + 12u] = extent;
        img[off + 13u] = 0u;
        img[off + 14u] = 0u;
        img[off + 15u] = 0x80u;
    };
    put(12352u, 0x00u, "WS      COM", 0u);
    put(12384u, 0x00u, "WS      COM", 1u);
    put(12416u, 0xE5u, "OLD     COM", 0u);

    const dumpfloppy::analysis a = analyse_bytes(img, "cpm.img");
    REQUIRE(a.cpm);
    REQUIRE_FALSE(a.apple.present);
    const dumpfloppy::dir_entry* ws = find_name(a, "WS.COM");
    REQUIRE(ws != nullptr);
    bool old_live = false;
    bool old_deleted = false;
    for (const dumpfloppy::dir_entry& e : a.entries)
    {
        if (e.name_83 == "OLD.COM" && !e.deleted)
        {
            old_live = true;
        }
        if (e.name_83 == "OLD.COM" && e.deleted)
        {
            old_deleted = true;
        }
    }
    REQUIRE_FALSE(old_live);
    REQUIRE(old_deleted);

    dumpfloppy::report_options opt;
    opt.color = false;
    opt.json = true;
    std::ostringstream js;
    dumpfloppy::write_json_begin(js);
    dumpfloppy::write_json_image(a, js, opt, true);
    dumpfloppy::write_json_end(js);
    REQUIRE(js.str().find("\"filesystem\": \"CP/M\"") != std::string::npos);
    REQUIRE(js.str().find("WS.COM") != std::string::npos);
}

TEST_CASE("WordStar D2_PROG.IMG lists WSOVLY1.OVR when the sample exists", "[ws040][sample]")
{
    const std::filesystem::path p{
        "/mnt/samples/WordStar 3.30 (5.25)/Images/Raw/D2_PROG.IMG"};
    if (!std::filesystem::exists(p))
    {
        SKIP("D2_PROG.IMG is not present");
    }
    auto loaded = dumpfloppy::load_image(p);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    const dumpfloppy::dir_entry* ovl = find_name(a, "WSOVLY1.OVR");
    REQUIRE(ovl != nullptr);
    REQUIRE(ovl->first_cluster == 2u);
}
