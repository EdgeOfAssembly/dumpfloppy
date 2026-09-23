/**
 * @file test_apple.cpp
 * @brief DOS 3.3 / ProDOS catalog, 2IMG wrap, extract, refuse -u.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/apple.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/update.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace
{

void put_le16(std::vector<uint8_t>& b, std::size_t off, uint16_t v)
{
    b[off] = static_cast<uint8_t>(v);
    b[off + 1u] = static_cast<uint8_t>(v >> 8);
}

void put_le32(std::vector<uint8_t>& b, std::size_t off, uint32_t v)
{
    b[off] = static_cast<uint8_t>(v);
    b[off + 1u] = static_cast<uint8_t>(v >> 8);
    b[off + 2u] = static_cast<uint8_t>(v >> 16);
    b[off + 3u] = static_cast<uint8_t>(v >> 24);
}

std::vector<uint8_t> make_dos33()
{
    std::vector<uint8_t> img(dumpfloppy::k_apple_dos33_140k, 0);
    constexpr std::size_t vtoc = 17u * 16u * 256u;
    img[vtoc + 1u] = 17;
    img[vtoc + 2u] = 15;
    img[vtoc + 6u] = 1;
    img[vtoc + 0x34u] = 35;
    img[vtoc + 0x35u] = 16;
    img[vtoc + 0x36u] = 0x00;
    img[vtoc + 0x37u] = 0x01;

    constexpr std::size_t cat = 17u * 16u * 256u + 15u * 256u;
    img[cat + 0x0Bu] = 18;
    img[cat + 0x0Cu] = 0;
    img[cat + 0x0Du] = 0x04;
    const char* name = "HELLO";
    for (int i = 0; i < 30; ++i)
    {
        const char c = (i < 5) ? name[i] : ' ';
        img[cat + 0x0Eu + static_cast<std::size_t>(i)] =
            static_cast<uint8_t>(static_cast<unsigned char>(c) | 0x80u);
    }
    put_le16(img, cat + 0x0Bu + 33u, 2);

    constexpr std::size_t ts = 18u * 16u * 256u;
    img[ts + 0x0Cu] = 18;
    img[ts + 0x0Du] = 1;
    constexpr std::size_t data = 18u * 16u * 256u + 256u;
    img[data] = 'H';
    img[data + 1u] = 'I';
    img[data + 2u] = '!';
    return img;
}

std::vector<uint8_t> make_prodos()
{
    std::vector<uint8_t> img(16u * 512u, 0);
    constexpr std::size_t b2 = 2u * 512u;
    img[b2 + 4u] = 0xF5;
    std::memcpy(img.data() + b2 + 5u, "APPLE", 5);
    img[b2 + 0x23u] = 0x27;
    img[b2 + 0x24u] = 0x0D;
    put_le16(img, b2 + 0x25u, 1);
    put_le16(img, b2 + 0x29u, 16);

    constexpr std::size_t e = b2 + 4u + 0x27u;
    img[e] = 0x15;
    std::memcpy(img.data() + e + 1u, "HELLO", 5);
    img[e + 0x10u] = 0x06;
    put_le16(img, e + 0x11u, 8);
    img[e + 0x15u] = 3;
    constexpr std::size_t data = 8u * 512u;
    img[data] = 'H';
    img[data + 1u] = 'I';
    img[data + 2u] = '!';
    return img;
}

std::vector<uint8_t> make_2img_prodos()
{
    const auto vol = make_prodos();
    std::vector<uint8_t> img(64u + vol.size(), 0);
    std::memcpy(img.data(), "2IMG", 4);
    std::memcpy(img.data() + 4, "TEST", 4);
    img[8] = 64;
    img[12] = 1;
    put_le32(img, 22, 64u);
    put_le32(img, 26, static_cast<uint32_t>(vol.size()));
    std::copy(vol.begin(), vol.end(), img.begin() + 64);
    return img;
}

dumpfloppy::floppy_image wrap(std::vector<uint8_t> bytes, const char* path)
{
    dumpfloppy::floppy_image img{};
    img.bytes = std::move(bytes);
    img.path = path;
    return img;
}

} /* namespace */

TEST_CASE("parse_apple DOS 3.3 lists BIN HELLO", "[apple][dos33]")
{
    const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(make_dos33());
    REQUIRE(d.present);
    REQUIRE(d.fs == dumpfloppy::apple_fs::dos33);
    REQUIRE(d.entries.size() == 1u);
    REQUIRE(d.entries[0].name == "HELLO");
    REQUIRE(d.entries[0].type_name == "BIN");
    const auto payload = dumpfloppy::read_apple_file(d, d.entries[0]);
    REQUIRE(payload.size() >= 3u);
    REQUIRE(payload[0] == 'H');
    REQUIRE(payload[2] == '!');
}

TEST_CASE("parse_apple ProDOS lists seedling HELLO", "[apple][prodos]")
{
    const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(make_prodos());
    REQUIRE(d.present);
    REQUIRE(d.fs == dumpfloppy::apple_fs::prodos);
    REQUIRE(d.volume_name == "APPLE");
    REQUIRE(d.entries.size() == 1u);
    REQUIRE(d.entries[0].name == "HELLO");
    REQUIRE(d.entries[0].type_name == "BIN");
    const auto payload = dumpfloppy::read_apple_file(d, d.entries[0]);
    REQUIRE(payload.size() == 3u);
    REQUIRE(payload[0] == 'H');
}

TEST_CASE("analyse DOS 3.3 skips FAT", "[apple][analyze]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(wrap(make_dos33(), "t.dsk"));
    REQUIRE(a.apple.present);
    REQUIRE_FALSE(a.bpb.looks_valid);
    dumpfloppy::report_options opt{};
    opt.color = false;
    opt.hex_boot = true;
    std::ostringstream plain;
    dumpfloppy::write_report(a, plain, opt);
    const std::string s = plain.str();
    REQUIRE(s.find("APPLE DOS 3.3") != std::string::npos);
    REQUIRE(s.find("HELLO") != std::string::npos);
    REQUIRE(s.find("BIOS PARAMETER BLOCK") == std::string::npos);
    REQUIRE(s.find("BOOT SECTOR HEX") == std::string::npos);
}

TEST_CASE("analyse 2IMG ProDOS lists catalog", "[apple][2img]")
{
    const dumpfloppy::analysis a =
        dumpfloppy::analyse(wrap(make_2img_prodos(), "t.2mg"));
    REQUIRE(a.foreign.present);
    REQUIRE(a.apple.present);
    REQUIRE(a.apple.fs == dumpfloppy::apple_fs::prodos);
    REQUIRE(a.apple.volume_name == "APPLE");
    dumpfloppy::report_options opt{};
    opt.color = false;
    std::ostringstream plain;
    dumpfloppy::write_report(a, plain, opt);
    const std::string s = plain.str();
    REQUIRE(s.find("APPLE 2IMG / ProDOS") != std::string::npos);
    REQUIRE(s.find("CONTAINER") != std::string::npos);
    REQUIRE(s.find("HELLO") != std::string::npos);
}

TEST_CASE("extract writes ProDOS seedling", "[apple][extract]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(wrap(make_prodos(), "t.po"));
    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "apple-out";
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);
    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 1);
    std::ifstream in(dest / "HELLO", std::ios::binary);
    std::string body((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
    REQUIRE(body == "HI!");
}

TEST_CASE("update refuses Apple volumes", "[apple][update]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(wrap(make_dos33(), "t.dsk"));
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.push_back("HELLO");
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) != 0);
    REQUIRE(err.str().find("Apple") != std::string::npos);
}

TEST_CASE("IBM 160K is not Apple DOS", "[apple]")
{
    std::vector<uint8_t> ibm(163840u, 0);
    REQUIRE_FALSE(dumpfloppy::parse_apple(ibm).present);
}

TEST_CASE("DOS-order to ProDOS-order exposes block-2 header", "[apple][prodos]")
{
    std::vector<uint8_t> po(dumpfloppy::k_apple_dos33_140k, 0);
    constexpr std::size_t b2 = 2u * 512u;
    po[b2 + 4u] = 0xF5;
    std::memcpy(po.data() + b2 + 5u, "APPLE", 5);
    po[b2 + 0x23u] = 0x27;
    po[b2 + 0x24u] = 0x0D;
    put_le16(po, b2 + 0x25u, 1);
    put_le16(po, b2 + 0x29u, 280);
    REQUIRE(dumpfloppy::parse_apple(po).present);
    REQUIRE(dumpfloppy::parse_apple(po).fs == dumpfloppy::apple_fs::prodos);

    std::vector<uint8_t> dos(po.size(), 0);
    static constexpr uint8_t k_map[16] = {
        0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15};
    constexpr std::size_t k_track = 16u * 256u;
    const std::size_t tracks = po.size() / k_track;
    for (std::size_t t = 0; t < tracks; ++t)
    {
        const std::size_t base = t * k_track;
        for (unsigned s = 0; s < 16u; ++s)
        {
            std::memcpy(dos.data() + base + static_cast<std::size_t>(k_map[s]) * 256u,
                        po.data() + base + static_cast<std::size_t>(s) * 256u, 256u);
        }
    }
    REQUIRE_FALSE(dumpfloppy::parse_apple(dos).present);
    const auto back = dumpfloppy::apple_dos_order_to_prodos(dos);
    REQUIRE(back.size() == po.size());
    const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(back);
    REQUIRE(d.present);
    REQUIRE(d.fs == dumpfloppy::apple_fs::prodos);
    REQUIRE(d.volume_name == "APPLE");
}
