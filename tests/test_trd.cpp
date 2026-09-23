/**
 * @file test_trd.cpp
 * @brief TR-DOS TRD parse, analyse skip-FAT, extract, refuse -u.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/trd.hpp"
#include "dumpfloppy/update.hpp"
#include "dumpfloppy/util.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace
{

std::vector<uint8_t> make_ss40_trd()
{
    std::vector<uint8_t> img(dumpfloppy::k_trd_ss40_bytes, 0);
    constexpr std::size_t info = 8u * 256u;
    img[info + 0xE1u] = 1;
    img[info + 0xE2u] = 1;
    img[info + 0xE3u] = 0x19u;
    img[info + 0xE4u] = 1;
    img[info + 0xE5u] = 0x70u;
    img[info + 0xE6u] = 0x02u;
    img[info + 0xE7u] = 0x10u;
    img[info + 0xECu] = 1;
    const char label[8] = {'T', 'E', 'S', 'T', 'D', 'I', 'S', 'K'};
    std::copy(label, label + 8, img.begin() + static_cast<std::ptrdiff_t>(info + 0xEDu));

    /* Live CODE file HELLO at logical track 1 sector 0. */
    const char name[8] = {'H', 'E', 'L', 'L', 'O', ' ', ' ', ' '};
    std::copy(name, name + 8, img.begin());
    img[8] = 'C';
    img[9] = 0x00;
    img[10] = 0x80;
    img[11] = 4;
    img[12] = 0;
    img[13] = 1;
    img[14] = 0;
    img[15] = 1;
    constexpr std::size_t payload = 16u * 256u;
    img[payload + 0] = 'T';
    img[payload + 1] = 'R';
    img[payload + 2] = 'D';
    img[payload + 3] = '!';

    /* Deleted CODE slot. */
    img[16] = 0x01u;
    const char rest[7] = {'L', 'D', 'F', 'I', 'L', 'E', ' '};
    std::copy(rest, rest + 7, img.begin() + 17);
    img[24] = 'C';
    img[25] = 0;
    img[26] = 0;
    img[27] = 1;
    img[28] = 0;
    img[29] = 1;
    img[30] = 1;
    img[31] = 1;
    img[payload + 256] = 'X';

    return img;
}

dumpfloppy::floppy_image make_trd_image()
{
    dumpfloppy::floppy_image img{};
    img.bytes = make_ss40_trd();
    img.path = "sample.trd";
    img.container = dumpfloppy::container_kind::trd_spectrum;
    img.xxh64 = dumpfloppy::xxh64_hex(img.bytes);
    return img;
}

std::filesystem::path write_temp_trd()
{
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto path = dir / "sample.trd";
    const auto bytes = make_ss40_trd();
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    REQUIRE(out);
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    REQUIRE(out);
    return path;
}

} /* namespace */

TEST_CASE("is_trd_image requires disk-info, not IBM 160K size", "[trd]")
{
    REQUIRE_FALSE(dumpfloppy::is_trd_image({}));
    std::vector<uint8_t> ibm160(163840u, 0);
    REQUIRE_FALSE(dumpfloppy::is_trd_image(ibm160));
    const auto trd = make_ss40_trd();
    REQUIRE(dumpfloppy::is_trd_image(trd));
}

TEST_CASE("parse_trd lists CODE file and deleted slot", "[trd]")
{
    const dumpfloppy::trd_disk disk = dumpfloppy::parse_trd(make_ss40_trd());
    REQUIRE(disk.present);
    REQUIRE(disk.disk_type == 0x19u);
    REQUIRE(disk.label == "TESTDISK");
    REQUIRE(disk.cylinders == 40);
    REQUIRE(disk.sides == 1);
    REQUIRE(disk.entries.size() == 2u);
    REQUIRE(disk.entries[0].name == "HELLO");
    REQUIRE(disk.entries[0].type_name == "CODE");
    REQUIRE_FALSE(disk.entries[0].deleted);
    REQUIRE(disk.entries[1].deleted);
    REQUIRE(disk.entries[1].name.front() == '?');
    const auto payload = dumpfloppy::read_trd_file(make_ss40_trd(), disk.entries[0]);
    REQUIRE(payload.size() == 4u);
    REQUIRE(payload[0] == 'T');
    REQUIRE(payload[3] == '!');
    REQUIRE(dumpfloppy::trd_host_filename(disk.entries[0]) == "HELLO.C");
}

TEST_CASE("analyse TRD skips FAT and reports TR-DOS", "[trd][analyze]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_trd_image());
    REQUIRE(a.trd.present);
    REQUIRE_FALSE(a.cbm.present);
    REQUIRE_FALSE(a.amiga.present);
    REQUIRE_FALSE(a.bpb.looks_valid);
    REQUIRE(a.entries.empty());
    dumpfloppy::report_options opt{};
    opt.color = false;
    opt.hex_boot = true;
    std::ostringstream plain;
    dumpfloppy::write_report(a, plain, opt);
    const std::string s = plain.str();
    REQUIRE(s.find("ZX TRD / TR-DOS") != std::string::npos);
    REQUIRE(s.find("TR-DOS") != std::string::npos);
    REQUIRE(s.find("TESTDISK") != std::string::npos);
    REQUIRE(s.find("HELLO") != std::string::npos);
    REQUIRE(s.find("CODE") != std::string::npos);
    REQUIRE(s.find("BIOS PARAMETER BLOCK") == std::string::npos);
    REQUIRE(s.find("BOOT SECTOR HEX") == std::string::npos);
    REQUIRE(s.find("; not FAT") == std::string::npos);
}

TEST_CASE("extract writes TRD CODE payload", "[trd][extract]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_trd_image());
    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "trd-out";
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);

    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 2);
    const auto hello = dest / "HELLO.C";
    REQUIRE(std::filesystem::exists(hello));
    std::ifstream in(hello, std::ios::binary);
    std::string body((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
    REQUIRE(body == "TRD!");
}

TEST_CASE("update refuses TRD", "[trd][update]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(make_trd_image());
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.push_back("HELLO.C");
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) != 0);
    REQUIRE(err.str().find("TRD") != std::string::npos);
}

TEST_CASE("load_image .trd sets trd_spectrum container", "[trd][image]")
{
    const auto path = write_temp_trd();
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded);
    REQUIRE(loaded->container == dumpfloppy::container_kind::trd_spectrum);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.trd.present);
}
