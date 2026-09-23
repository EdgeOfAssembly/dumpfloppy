/**
 * @file test_cbm.cpp
 * @brief D64/D71/D81 geometry, CBMFS parse, and file-chain round-trip.
 */
#include "d64_builder.hpp"
#include "d71_builder.hpp"
#include "d81_builder.hpp"
#include "dumpfloppy/cbm.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace
{

std::vector<uint8_t> slurp(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    REQUIRE(in);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in),
                                std::istreambuf_iterator<char>());
}

const dumpfloppy::cbm_file* find_name(const dumpfloppy::cbm_disk& disk,
                                      const std::string& name)
{
    for (const dumpfloppy::cbm_file& e : disk.entries)
    {
        if (e.name == name)
        {
            return &e;
        }
    }
    return nullptr;
}

} /* namespace */

static_assert(dumpfloppy::d64_offset(18, 0) == 17u * 21u * 256u);
static_assert(dumpfloppy::d64_offset(1, 0) == 0u);
static_assert(dumpfloppy::k_d64_35_bytes == 683u * 256u);
static_assert(dumpfloppy::d64_sectors_per_track(1) == 21u);
static_assert(dumpfloppy::d64_sectors_per_track(18) == 19u);
static_assert(dumpfloppy::d64_sectors_per_track(25) == 18u);
static_assert(dumpfloppy::d64_sectors_per_track(35) == 17u);
static_assert(!dumpfloppy::d64_ts_valid(0, 0));
static_assert(!dumpfloppy::d64_ts_valid(18, 19));
static_assert(dumpfloppy::d64_ts_valid(18, 18));
static_assert(dumpfloppy::k_d71_bytes == 1366u * 256u);
static_assert(dumpfloppy::k_d71_error_bytes == 349696u + 1366u);
static_assert(dumpfloppy::k_d81_bytes == 80u * 40u * 256u);
static_assert(dumpfloppy::k_d81_error_bytes == 819200u + 3200u);
static_assert(dumpfloppy::cbm_offset(dumpfloppy::cbm_media::d71, 36, 0) ==
              683u * 256u);
static_assert(dumpfloppy::cbm_offset(dumpfloppy::cbm_media::d71, 53, 0) ==
              1040u * 256u);
static_assert(dumpfloppy::cbm_offset(dumpfloppy::cbm_media::d81, 40, 0) ==
              39u * 40u * 256u);
static_assert(dumpfloppy::cbm_sectors_per_track(dumpfloppy::cbm_media::d71, 36) ==
              21u);
static_assert(dumpfloppy::cbm_sectors_per_track(dumpfloppy::cbm_media::d71, 70) ==
              17u);
static_assert(dumpfloppy::cbm_sectors_per_track(dumpfloppy::cbm_media::d81, 1) ==
              40u);
static_assert(dumpfloppy::d64_offset(dumpfloppy::cbm_media::d71, 36, 0) ==
              683u * 256u);
static_assert(dumpfloppy::d64_offset(36, 0) == static_cast<std::size_t>(-1));
static_assert(dumpfloppy::cbm_media_from_size(dumpfloppy::k_d71_bytes) ==
              dumpfloppy::cbm_media::d71);
static_assert(dumpfloppy::cbm_media_from_size(dumpfloppy::k_d81_bytes) ==
              dumpfloppy::cbm_media::d81);

TEST_CASE("d64_offset matches 1541 zone table", "[cbm][d64]")
{
    using dumpfloppy::d64_offset;
    REQUIRE(d64_offset(18, 0) == 17u * 21u * 256u);
    REQUIRE(d64_offset(1, 0) == 0u);
    REQUIRE(d64_offset(19, 0) == (17u * 21u + 19u) * 256u);
    REQUIRE(d64_offset(35, 16) == (666u + 16u) * 256u);
    REQUIRE(d64_offset(35, 16) + 256u == dumpfloppy::k_d64_35_bytes);
    REQUIRE(d64_offset(0, 0) == static_cast<std::size_t>(-1));
    REQUIRE(d64_offset(36, 0) == static_cast<std::size_t>(-1));
    REQUIRE(d64_offset(18, 19) == static_cast<std::size_t>(-1));
    REQUIRE(d64_offset(17, 20) != static_cast<std::size_t>(-1));
    REQUIRE(d64_offset(17, 21) == static_cast<std::size_t>(-1));
}

TEST_CASE("petscii_to_ascii strips 0xA0 and keeps A-Z 0-9 dash", "[cbm]")
{
    const uint8_t raw[] = {'T', 'E', 'S', 'T', '-', '1', 0xA0, 0xA0, 'X'};
    REQUIRE(dumpfloppy::petscii_to_ascii(raw) == "TEST-1");
    const uint8_t shifted[] = {0xC8, 0xC5, 0xCC, 0xCC, 0xCF, 0xA0};
    REQUIRE(dumpfloppy::petscii_to_ascii(shifted) == "HELLO");
    REQUIRE(dumpfloppy::cbm_file_kind_name(dumpfloppy::cbm_file_kind::prg) ==
            std::string("PRG"));
    REQUIRE(dumpfloppy::cbm_file_kind_name(dumpfloppy::cbm_file_kind::seq) ==
            std::string("SEQ"));
}

TEST_CASE("parse_d64 rejects wrong size and invalid BAM", "[cbm][d64]")
{
    std::vector<uint8_t> tiny(256, 0);
    REQUIRE_FALSE(dumpfloppy::parse_d64(tiny).present);
    REQUIRE_FALSE(dumpfloppy::parse_cbmfs(tiny).present);

    std::vector<uint8_t> fat_size(737280, 0);
    REQUIRE_FALSE(dumpfloppy::parse_d64(fat_size).present);

    auto img = dumpfloppy_test::make_sample_d64();
    REQUIRE(img.size() == 174848u);
    uint8_t* bam = img.data() + dumpfloppy::d64_offset(18, 0);
    bam[2] = static_cast<uint8_t>('X');
    REQUIRE_FALSE(dumpfloppy::parse_d64(img).present);
    REQUIRE_FALSE(dumpfloppy::parse_cbmfs(img).present);
}

TEST_CASE("synthetic D64 parses name, ID, PRG and deleted SEQ", "[cbm][d64]")
{
    const auto img = dumpfloppy_test::make_sample_d64();
    REQUIRE(img.size() == 174848u);

    const dumpfloppy::cbm_disk d64 = dumpfloppy::parse_d64(img);
    REQUIRE(d64.present);
    REQUIRE(d64.media == dumpfloppy::cbm_media::d64);
    REQUIRE(d64.media_name == "D64");
    REQUIRE(d64.disk_name == "TEST DISK");
    REQUIRE(d64.disk_id == "DF");
    REQUIRE(d64.dos_version == static_cast<uint8_t>('A'));
    REQUIRE(d64.dos_type == "2A");
    REQUIRE(d64.dir_track == 18);
    REQUIRE(d64.dir_sector == 1);
    REQUIRE(d64.entries.size() == 2u);

    const dumpfloppy::cbm_disk fs = dumpfloppy::parse_cbmfs(img);
    REQUIRE(fs.present);
    REQUIRE(fs.media == dumpfloppy::cbm_media::d64);
    REQUIRE(fs.entries.size() == d64.entries.size());
    REQUIRE(fs.disk_name == d64.disk_name);

    const dumpfloppy::cbm_disk picked = dumpfloppy::parse_cbm_image(img);
    REQUIRE(picked.present);
    REQUIRE(picked.media == dumpfloppy::cbm_media::d64);
    REQUIRE(picked.entries.size() == d64.entries.size());

    const dumpfloppy::cbm_file* prg = find_name(d64, "HELLO");
    REQUIRE(prg != nullptr);
    REQUIRE(prg->kind == dumpfloppy::cbm_file_kind::prg);
    REQUIRE(prg->type_byte == 0x82u);
    REQUIRE_FALSE(prg->deleted);
    REQUIRE(prg->closed);
    REQUIRE(prg->first_track == 17);
    REQUIRE(prg->first_sector == 0);
    REQUIRE(prg->size_sectors == 2u);

    const dumpfloppy::cbm_file* seq = find_name(d64, "OLDSEQ");
    REQUIRE(seq != nullptr);
    REQUIRE(seq->kind == dumpfloppy::cbm_file_kind::seq);
    REQUIRE(seq->type_byte == 0x01u);
    REQUIRE(seq->deleted);
    REQUIRE_FALSE(seq->closed);
    REQUIRE(seq->first_track == 17);
    REQUIRE(seq->first_sector == 2);
    REQUIRE(seq->size_sectors == 1u);

    const std::vector<uint8_t> prg_bytes =
        dumpfloppy::read_cbm_file(img, *prg);
    REQUIRE(prg_bytes == dumpfloppy_test::sample_prg_bytes());
    REQUIRE(prg_bytes.size() == 300u);

    const std::vector<uint8_t> seq_bytes =
        dumpfloppy::read_cbm_file(img, *seq);
    REQUIRE(seq_bytes == dumpfloppy_test::sample_seq_bytes());
}

TEST_CASE("read_cbm_file last sector uses S as last used byte", "[cbm]")
{
    auto img = dumpfloppy_test::make_sample_d64();
    const std::size_t off = dumpfloppy::d64_offset(17, 2);
    /* SEQ payload "SEQ-DATA" is 8 bytes → last used byte = 1+8 = 9. */
    REQUIRE(img[off] == 0);
    REQUIRE(img[off + 1u] == 9u);
    const std::vector<uint8_t> got = dumpfloppy::read_cbm_file(img, 17, 2);
    REQUIRE(got.size() == 8u);
    REQUIRE(got == dumpfloppy_test::sample_seq_bytes());
}

TEST_CASE("Last Ninja D64 BAM looks like CBMFS", "[cbm][d64][lastninja]")
{
    const std::filesystem::path img_path{"/mnt/RetroCodeMess/c64/last_ninja.d64"};
    if (!std::filesystem::exists(img_path))
    {
        SKIP("Last Ninja .d64 is not present");
    }
    const std::vector<uint8_t> bytes = slurp(img_path);
    REQUIRE(bytes.size() == 174848u);
    const dumpfloppy::cbm_disk disk = dumpfloppy::parse_d64(bytes);
    REQUIRE(disk.present);
    REQUIRE(disk.dos_version == static_cast<uint8_t>('A'));
    REQUIRE(disk.dir_track == 18);
    REQUIRE(disk.dir_sector == 1);
    REQUIRE(disk.disk_name.find("DIGITAL") != std::string::npos);
    REQUIRE(disk.disk_id == "TD");
}

TEST_CASE("d71/d81 geometry tables and 1541 wrappers", "[cbm][d71][d81]")
{
    using dumpfloppy::cbm_media;
    using dumpfloppy::cbm_offset;
    using dumpfloppy::cbm_sectors_per_track;
    using dumpfloppy::d64_offset;

    REQUIRE(cbm_offset(cbm_media::d71, 1, 0) == 0u);
    REQUIRE(cbm_offset(cbm_media::d71, 18, 0) == 17u * 21u * 256u);
    REQUIRE(cbm_offset(cbm_media::d71, 36, 0) == 683u * 256u);
    REQUIRE(cbm_offset(cbm_media::d71, 36, 0) == dumpfloppy::k_d64_35_bytes);
    REQUIRE(cbm_offset(cbm_media::d71, 70, 16) + 256u == dumpfloppy::k_d71_bytes);
    REQUIRE(cbm_sectors_per_track(cbm_media::d71, 36) == 21u);
    REQUIRE(cbm_sectors_per_track(cbm_media::d71, 53) == 19u);
    REQUIRE(cbm_sectors_per_track(cbm_media::d71, 70) == 17u);
    REQUIRE(cbm_offset(cbm_media::d71, 71, 0) == static_cast<std::size_t>(-1));

    REQUIRE(cbm_offset(cbm_media::d81, 1, 0) == 0u);
    REQUIRE(cbm_offset(cbm_media::d81, 40, 0) == 39u * 40u * 256u);
    REQUIRE(cbm_offset(cbm_media::d81, 80, 39) + 256u == dumpfloppy::k_d81_bytes);
    REQUIRE(cbm_sectors_per_track(cbm_media::d81, 40) == 40u);
    REQUIRE(cbm_offset(cbm_media::d81, 40, 40) == static_cast<std::size_t>(-1));
    REQUIRE(cbm_offset(cbm_media::d81, 81, 0) == static_cast<std::size_t>(-1));

    /* 1541 wrappers stay 35-track; geometry-aware overloads accept D71/D81. */
    REQUIRE(d64_offset(36, 0) == static_cast<std::size_t>(-1));
    REQUIRE(d64_offset(cbm_media::d71, 36, 0) == 683u * 256u);
    REQUIRE(d64_offset(cbm_media::d81, 40, 0) == 39u * 40u * 256u);
}

TEST_CASE("parse_d71/d81 reject wrong size and invalid header", "[cbm][d71][d81]")
{
    std::vector<uint8_t> tiny(256, 0);
    REQUIRE_FALSE(dumpfloppy::parse_d71(tiny).present);
    REQUIRE_FALSE(dumpfloppy::parse_d81(tiny).present);
    REQUIRE_FALSE(dumpfloppy::parse_cbm_image(tiny).present);

    const auto d64 = dumpfloppy_test::make_sample_d64();
    REQUIRE_FALSE(dumpfloppy::parse_d71(d64).present);
    REQUIRE_FALSE(dumpfloppy::parse_d81(d64).present);

    auto d71 = dumpfloppy_test::make_sample_d71();
    REQUIRE(d71.size() == 349696u);
    uint8_t* bam = d71.data() + dumpfloppy::cbm_offset(dumpfloppy::cbm_media::d71, 18, 0);
    bam[2] = static_cast<uint8_t>('X');
    REQUIRE_FALSE(dumpfloppy::parse_d71(d71).present);
    REQUIRE_FALSE(dumpfloppy::parse_cbmfs(d71, dumpfloppy::cbm_media::d71).present);

    auto d81 = dumpfloppy_test::make_sample_d81();
    REQUIRE(d81.size() == 819200u);
    uint8_t* hdr = d81.data() + dumpfloppy::cbm_offset(dumpfloppy::cbm_media::d81, 40, 0);
    hdr[2] = static_cast<uint8_t>('A');
    REQUIRE_FALSE(dumpfloppy::parse_d81(d81).present);
    REQUIRE_FALSE(dumpfloppy::parse_cbmfs(d81, dumpfloppy::cbm_media::d81).present);

    auto d81_bam = dumpfloppy_test::make_sample_d81();
    uint8_t* bam81 = d81_bam.data() + dumpfloppy::cbm_offset(dumpfloppy::cbm_media::d81, 40, 1);
    bam81[3] = 0x00; /* not ~'D' */
    REQUIRE_FALSE(dumpfloppy::parse_d81(d81_bam).present);
}

TEST_CASE("synthetic D71 BAM 18/0 PRG on track 36 round-trip", "[cbm][d71]")
{
    const auto img = dumpfloppy_test::make_sample_d71();
    REQUIRE(img.size() == 349696u);

    const dumpfloppy::cbm_disk d71 = dumpfloppy::parse_d71(img);
    REQUIRE(d71.present);
    REQUIRE(d71.media == dumpfloppy::cbm_media::d71);
    REQUIRE(d71.media_name == "D71");
    REQUIRE(d71.disk_name == "TEST 1571");
    REQUIRE(d71.disk_id == "71");
    REQUIRE(d71.dos_version == static_cast<uint8_t>('A'));
    REQUIRE(d71.dos_type == "2A");
    REQUIRE(d71.dir_track == 18);
    REQUIRE(d71.dir_sector == 1);
    REQUIRE(d71.entries.size() == 1u);

    const dumpfloppy::cbm_disk via_image = dumpfloppy::parse_cbm_image(img);
    REQUIRE(via_image.present);
    REQUIRE(via_image.media == dumpfloppy::cbm_media::d71);
    REQUIRE(via_image.entries.size() == 1u);

    const dumpfloppy::cbm_disk via_fs = dumpfloppy::parse_cbmfs(img);
    REQUIRE(via_fs.present);
    REQUIRE(via_fs.media == dumpfloppy::cbm_media::d71);

    const dumpfloppy::cbm_file* prg = find_name(d71, "SIDE1");
    REQUIRE(prg != nullptr);
    REQUIRE(prg->kind == dumpfloppy::cbm_file_kind::prg);
    REQUIRE(prg->type_byte == 0x82u);
    REQUIRE_FALSE(prg->deleted);
    REQUIRE(prg->first_track == 36);
    REQUIRE(prg->first_sector == 0);
    REQUIRE(prg->size_sectors == 2u);

    const std::vector<uint8_t> prg_bytes = dumpfloppy::read_cbm_file(img, *prg);
    REQUIRE(prg_bytes == dumpfloppy_test::sample_d71_prg_bytes());
    REQUIRE(prg_bytes.size() == 300u);

    const std::vector<uint8_t> explicit_media = dumpfloppy::read_cbm_file(
        img, dumpfloppy::cbm_media::d71, 36, 0);
    REQUIRE(explicit_media == prg_bytes);
}

TEST_CASE("synthetic D81 header 40/0 dir 40/3 PRG round-trip", "[cbm][d81]")
{
    const auto img = dumpfloppy_test::make_sample_d81();
    REQUIRE(img.size() == 819200u);

    const dumpfloppy::cbm_disk d81 = dumpfloppy::parse_d81(img);
    REQUIRE(d81.present);
    REQUIRE(d81.media == dumpfloppy::cbm_media::d81);
    REQUIRE(d81.media_name == "D81");
    REQUIRE(d81.disk_name == "TEST 1581");
    REQUIRE(d81.disk_id == "81");
    REQUIRE(d81.dos_version == static_cast<uint8_t>('D'));
    REQUIRE(d81.dos_type == "3D");
    REQUIRE(d81.dir_track == 40);
    REQUIRE(d81.dir_sector == 3);
    REQUIRE(d81.entries.size() == 1u);

    const dumpfloppy::cbm_disk via_image = dumpfloppy::parse_cbm_image(img);
    REQUIRE(via_image.present);
    REQUIRE(via_image.media == dumpfloppy::cbm_media::d81);
    REQUIRE(via_image.disk_name == "TEST 1581");

    const dumpfloppy::cbm_file* prg = find_name(d81, "HELLO81");
    REQUIRE(prg != nullptr);
    REQUIRE(prg->kind == dumpfloppy::cbm_file_kind::prg);
    REQUIRE(prg->type_byte == 0x82u);
    REQUIRE_FALSE(prg->deleted);
    REQUIRE(prg->first_track == 1);
    REQUIRE(prg->first_sector == 0);
    REQUIRE(prg->size_sectors == 2u);

    const std::vector<uint8_t> prg_bytes = dumpfloppy::read_cbm_file(img, *prg);
    REQUIRE(prg_bytes == dumpfloppy_test::sample_d81_prg_bytes());
    REQUIRE(prg_bytes.size() == 300u);

    const std::vector<uint8_t> explicit_media = dumpfloppy::read_cbm_file(
        img, dumpfloppy::cbm_media::d81, *prg);
    REQUIRE(explicit_media == prg_bytes);
}
