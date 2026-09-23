/**
 * @file test_amiga.cpp
 * @brief ADF geometry, OFS/FFS parse, and file-extract round-trip.
 */
#include "adf_builder.hpp"
#include "dumpfloppy/amiga.hpp"

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

const dumpfloppy::amiga_file* find_path(const dumpfloppy::amiga_disk& disk,
                                        const std::string& path)
{
    for (const dumpfloppy::amiga_file& e : disk.entries)
    {
        if (e.path == path)
        {
            return &e;
        }
    }
    return nullptr;
}

} /* namespace */

static_assert(dumpfloppy::k_adf_dd_bytes == 80u * 2u * 11u * 512u);
static_assert(dumpfloppy::k_adf_hd_bytes == 80u * 2u * 22u * 512u);
static_assert(dumpfloppy::k_adf_dd_sectors == 1760u);
static_assert(dumpfloppy::k_adf_hd_sectors == 3520u);
static_assert(dumpfloppy::adf_root_block(1760u) == 880u);
static_assert(dumpfloppy::adf_root_block(3520u) == 1760u);
static_assert(!dumpfloppy::amiga_dos_is_ffs(0));
static_assert(dumpfloppy::amiga_dos_is_ffs(1));
static_assert(!dumpfloppy::amiga_dos_is_ffs(2));
static_assert(dumpfloppy::amiga_dos_is_ffs(5));
static_assert(dumpfloppy::is_adf_size(901120u));
static_assert(dumpfloppy::is_adf_size(1802240u));
static_assert(!dumpfloppy::is_adf_size(737280u));

TEST_CASE("ADF geometry and DOS type parity", "[amiga][adf]")
{
    using dumpfloppy::adf_root_block;
    using dumpfloppy::amiga_dos_is_ffs;
    using dumpfloppy::amiga_fs_name;
    using dumpfloppy::k_adf_dd_bytes;
    using dumpfloppy::k_adf_hd_bytes;

    REQUIRE(k_adf_dd_bytes == 80u * 2u * 11u * 512u);
    REQUIRE(k_adf_hd_bytes == 80u * 2u * 22u * 512u);
    REQUIRE(adf_root_block(1760u) == 880u);
    REQUIRE(adf_root_block(3520u) == 1760u);
    REQUIRE_FALSE(amiga_dos_is_ffs(0));
    REQUIRE(amiga_dos_is_ffs(1));
    REQUIRE_FALSE(amiga_dos_is_ffs(4));
    REQUIRE(amiga_dos_is_ffs(5));
    REQUIRE(std::string(amiga_fs_name(false)) == "OFS");
    REQUIRE(std::string(amiga_fs_name(true)) == "FFS");
}

TEST_CASE("parse_adf rejects wrong size and invalid boot/root", "[amiga][adf]")
{
    std::vector<uint8_t> tiny(256, 0);
    REQUIRE_FALSE(dumpfloppy::parse_adf(tiny).present);

    std::vector<uint8_t> fat_size(737280, 0);
    REQUIRE_FALSE(dumpfloppy::parse_adf(fat_size).present);

    auto img = dumpfloppy_test::make_ofs_root_file_adf();
    REQUIRE(img.size() == dumpfloppy::k_adf_dd_bytes);
    img[3] = 6;
    REQUIRE_FALSE(dumpfloppy::parse_adf(img).present);

    img = dumpfloppy_test::make_ofs_root_file_adf();
    img[0] = static_cast<uint8_t>('X');
    REQUIRE_FALSE(dumpfloppy::parse_adf(img).present);

    img = dumpfloppy_test::make_ofs_root_file_adf();
    const std::size_t roff =
        static_cast<std::size_t>(dumpfloppy::adf_root_block(dumpfloppy::k_adf_dd_sectors)) *
        static_cast<std::size_t>(dumpfloppy::k_adf_sector_bytes);
    img[roff] = 0;
    img[roff + 1u] = 0;
    img[roff + 2u] = 0;
    img[roff + 3u] = 0;
    REQUIRE_FALSE(dumpfloppy::parse_adf(img).present);

    img = dumpfloppy_test::make_ofs_root_file_adf();
    img[roff + 508u] = 0;
    img[roff + 509u] = 0;
    img[roff + 510u] = 0;
    img[roff + 511u] = 0;
    REQUIRE_FALSE(dumpfloppy::parse_adf(img).present);
}

TEST_CASE("DOS\\0 OFS root name and file extract match", "[amiga][adf][ofs]")
{
    const auto img = dumpfloppy_test::make_ofs_root_file_adf();
    REQUIRE(img.size() == dumpfloppy::k_adf_dd_bytes);
    REQUIRE(img[0] == static_cast<uint8_t>('D'));
    REQUIRE(img[3] == 0);

    const dumpfloppy::amiga_disk disk = dumpfloppy::parse_adf(img);
    REQUIRE(disk.present);
    REQUIRE_FALSE(disk.ffs);
    REQUIRE(disk.dos_type == 0);
    REQUIRE(disk.volume_name == "TESTADF");
    REQUIRE(disk.root_block == 880u);
    REQUIRE(disk.sector_count == dumpfloppy::k_adf_dd_sectors);

    const dumpfloppy::amiga_file* file = find_path(disk, "README");
    REQUIRE(file != nullptr);
    REQUIRE_FALSE(file->is_dir);
    REQUIRE(file->name == "README");
    REQUIRE(file->byte_size == dumpfloppy_test::sample_ofs_payload().size());
    REQUIRE(file->parent_block == disk.root_block);

    const std::vector<uint8_t> got = dumpfloppy::read_amiga_file(img, disk, *file);
    REQUIRE(got == dumpfloppy_test::sample_ofs_payload());
    REQUIRE(got.size() == 500u);
    REQUIRE(got[0] == static_cast<uint8_t>('O'));
    REQUIRE(dumpfloppy::amiga_host_filename(*file) == "README");
}

TEST_CASE("OFS subdirectory plus nested file", "[amiga][adf][ofs]")
{
    const auto img = dumpfloppy_test::make_ofs_subdir_adf();
    const dumpfloppy::amiga_disk disk = dumpfloppy::parse_adf(img);
    REQUIRE(disk.present);
    REQUIRE(disk.volume_name == "TESTADF");

    const dumpfloppy::amiga_file* dir = find_path(disk, "Sub");
    REQUIRE(dir != nullptr);
    REQUIRE(dir->is_dir);
    REQUIRE(dir->name == "Sub");

    const dumpfloppy::amiga_file* file = find_path(disk, "Sub/Inner");
    REQUIRE(file != nullptr);
    REQUIRE_FALSE(file->is_dir);
    REQUIRE(file->name == "Inner");
    REQUIRE(file->parent_block == dir->header_block);

    const std::vector<uint8_t> got = dumpfloppy::read_amiga_file(img, disk, *file);
    REQUIRE(got == dumpfloppy_test::sample_inner_payload());
    REQUIRE(dumpfloppy::amiga_host_filename(*file) == "Sub_Inner");
    REQUIRE(dumpfloppy::read_amiga_file(img, disk, *dir).empty());
}

TEST_CASE("DOS\\1 FFS raw 512-byte data blocks", "[amiga][adf][ffs]")
{
    const auto img = dumpfloppy_test::make_ffs_root_file_adf();
    const dumpfloppy::amiga_disk disk = dumpfloppy::parse_adf(img);
    REQUIRE(disk.present);
    REQUIRE(disk.ffs);
    REQUIRE(disk.dos_type == 1);
    REQUIRE(disk.volume_name == "FFSVOL");
    REQUIRE(std::string(dumpfloppy::amiga_fs_name(disk.ffs)) == "FFS");

    const dumpfloppy::amiga_file* file = find_path(disk, "RAWFILE");
    REQUIRE(file != nullptr);
    const std::vector<uint8_t> got = dumpfloppy::read_amiga_file(img, disk, *file);
    REQUIRE(got == dumpfloppy_test::sample_ffs_payload());
    REQUIRE(got.size() == 600u);
    REQUIRE(got[0] == static_cast<uint8_t>('F'));
    REQUIRE(got[512] == 0xBBu);
    REQUIRE(got[513] == 0xCCu);
}

TEST_CASE("FFS file follows extension block at offset 504", "[amiga][adf][ffs]")
{
    const auto img = dumpfloppy_test::make_ffs_extension_adf();
    const dumpfloppy::amiga_disk disk = dumpfloppy::parse_adf(img);
    REQUIRE(disk.present);
    const dumpfloppy::amiga_file* file = find_path(disk, "BIGFILE");
    REQUIRE(file != nullptr);
    REQUIRE(file->byte_size == 72u * 512u + 16u);
    const std::vector<uint8_t> got = dumpfloppy::read_amiga_file(img, disk, *file);
    REQUIRE(got.size() == file->byte_size);
    REQUIRE(got[0] == static_cast<uint8_t>('E'));
    REQUIRE(got[1] == static_cast<uint8_t>('X'));
    REQUIRE(got[2] == static_cast<uint8_t>('T'));
    REQUIRE(got[72u * 512u] == 0xEEu);
}

TEST_CASE("HD ADF rootblock is sector_count/2", "[amiga][adf]")
{
    const auto img = dumpfloppy_test::make_hd_ofs_adf();
    REQUIRE(img.size() == dumpfloppy::k_adf_hd_bytes);
    const dumpfloppy::amiga_disk disk = dumpfloppy::parse_adf(img);
    REQUIRE(disk.present);
    REQUIRE_FALSE(disk.ffs);
    REQUIRE(disk.root_block == 1760u);
    REQUIRE(disk.sector_count == dumpfloppy::k_adf_hd_sectors);
    REQUIRE(disk.volume_name == "HDROOT");
    const dumpfloppy::amiga_file* file = find_path(disk, "HDFILE");
    REQUIRE(file != nullptr);
    const std::vector<uint8_t> got = dumpfloppy::read_amiga_file(img, disk, *file);
    REQUIRE(got.size() == 7u);
    REQUIRE(std::string(got.begin(), got.end()) == "HD-FILE");
}

TEST_CASE("hash_chain at offset 496 walks same-slot files", "[amiga][adf]")
{
    const auto img = dumpfloppy_test::make_ofs_hash_chain_adf();
    const dumpfloppy::amiga_disk disk = dumpfloppy::parse_adf(img);
    REQUIRE(disk.present);
    const dumpfloppy::amiga_file* a = find_path(disk, "Alpha");
    const dumpfloppy::amiga_file* b = find_path(disk, "Beta");
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    REQUIRE(dumpfloppy::read_amiga_file(img, disk, *a) ==
            (std::vector<uint8_t>{'A', 'A', 'A'}));
    REQUIRE(dumpfloppy::read_amiga_file(img, disk, *b) ==
            (std::vector<uint8_t>{'C', 'C', 'C'}));
}

TEST_CASE("Beast_Sonix ADF volume and files", "[amiga][adf][beast]")
{
    const std::filesystem::path img_path{
        "/mnt/music/_src/sotb/Beast_Sonix_1990_Scoopex.adf"};
    if (!std::filesystem::exists(img_path))
    {
        SKIP("Beast_Sonix ADF is not present");
    }
    const std::vector<uint8_t> bytes = slurp(img_path);
    REQUIRE(bytes.size() == dumpfloppy::k_adf_dd_bytes);
    const dumpfloppy::amiga_disk disk = dumpfloppy::parse_adf(bytes);
    REQUIRE(disk.present);
    REQUIRE_FALSE(disk.ffs);
    REQUIRE(disk.dos_type == 0);
    REQUIRE(disk.root_block == 880u);
    const bool name_ok = disk.volume_name.find("Scoopex") != std::string::npos ||
                         disk.volume_name.find("Beast") != std::string::npos;
    REQUIRE(name_ok);

    const dumpfloppy::amiga_file* startup = find_path(disk, "S/Startup-Sequence");
    const dumpfloppy::amiga_file* shadow = find_path(disk, "Shadow0");
    REQUIRE((startup != nullptr || shadow != nullptr));

    if (startup != nullptr)
    {
        const std::vector<uint8_t> got =
            dumpfloppy::read_amiga_file(bytes, disk, *startup);
        REQUIRE(got.size() == 30u);
        const std::string text(got.begin(), got.end());
        REQUIRE(text.find("BeastSonix") != std::string::npos);
    }
    if (shadow != nullptr)
    {
        const std::vector<uint8_t> got =
            dumpfloppy::read_amiga_file(bytes, disk, *shadow);
        REQUIRE(got.size() == shadow->byte_size);
        REQUIRE(got.size() == 78232u);
    }
}
