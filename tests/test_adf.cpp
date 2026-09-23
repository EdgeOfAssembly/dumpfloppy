/**
 * @file test_adf.cpp
 * @brief Product wiring: analyse / listing / extract / refuse -u on Amiga ADF.
 */
#include "adf_builder.hpp"
#include "dumpfloppy/amiga.hpp"
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/update.hpp"
#include "image_builder.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace
{

dumpfloppy::floppy_image make_ofs_image()
{
    dumpfloppy::floppy_image img{};
    img.bytes = dumpfloppy_test::make_ofs_root_file_adf();
    img.path = "sample.adf";
    img.container = dumpfloppy::container_kind::adf_amiga;
    return img;
}

dumpfloppy::floppy_image make_ffs_image()
{
    dumpfloppy::floppy_image img{};
    img.bytes = dumpfloppy_test::make_ffs_root_file_adf();
    img.path = "ffs.adf";
    img.container = dumpfloppy::container_kind::adf_amiga;
    return img;
}

dumpfloppy::floppy_image make_subdir_image()
{
    dumpfloppy::floppy_image img{};
    img.bytes = dumpfloppy_test::make_ofs_subdir_adf();
    img.path = "sub.adf";
    img.container = dumpfloppy::container_kind::adf_amiga;
    return img;
}

} /* namespace */

TEST_CASE("analyse synthetic OFS ADF skips FAT", "[adf][analyse]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(make_ofs_image());
    REQUIRE(a.amiga.present);
    REQUIRE_FALSE(a.cbm.present);
    REQUIRE_FALSE(a.amiga.ffs);
    REQUIRE(a.amiga.dos_type == 0);
    REQUIRE(a.amiga.volume_name == "TESTADF");
    REQUIRE_FALSE(a.bpb.looks_valid);
    REQUIRE(a.kind == dumpfloppy::fat_kind::unknown);
    REQUIRE(a.entries.empty());
    REQUIRE_FALSE(a.flux.present);
    REQUIRE(a.image.size_geometry.cylinders == 80);
    REQUIRE(a.image.size_geometry.heads == 2);
    REQUIRE(a.image.size_geometry.sectors_per_track == 11);
    REQUIRE(a.image.size_geometry.bytes_per_sector == 512);
    REQUIRE(a.image.size_geometry.media_name.find("Amiga") != std::string::npos);

    bool saw_readme = false;
    for (const dumpfloppy::amiga_file& e : a.amiga.entries)
    {
        if (e.path == "README")
        {
            saw_readme = true;
            REQUIRE_FALSE(e.is_dir);
            REQUIRE(e.byte_size == dumpfloppy_test::sample_ofs_payload().size());
        }
    }
    REQUIRE(saw_readme);
}

TEST_CASE("FAT12 sample is not CBMFS and not ADF", "[adf][fat]")
{
    dumpfloppy::floppy_image img{};
    img.bytes = dumpfloppy_test::make_fat12_sample();
    img.path = "sample.ima";
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(img));
    REQUIRE_FALSE(a.cbm.present);
    REQUIRE_FALSE(a.amiga.present);
    REQUIRE(a.bpb.looks_valid);
    REQUIRE(a.kind == dumpfloppy::fat_kind::fat12);
}

TEST_CASE("write_report lists OFS ADF not FAT", "[adf][report]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_ofs_image());
    dumpfloppy::report_options opt{};
    opt.color = false;
    opt.hex_boot = true;
    std::ostringstream plain;
    dumpfloppy::write_report(a, plain, opt);
    const std::string s = plain.str();
    REQUIRE(s.find("AMIGA ADF / OFS") != std::string::npos);
    REQUIRE(s.find("OFS (Amiga)") != std::string::npos);
    REQUIRE(s.find("; not FAT") == std::string::npos);
    REQUIRE(s.find("TESTADF") != std::string::npos);
    REQUIRE(s.find("README") != std::string::npos);
    REQUIRE(s.find("AMIGA VOLUME") != std::string::npos);
    REQUIRE(s.find("BIOS PARAMETER BLOCK") == std::string::npos);
    REQUIRE(s.find("BOOT SECTOR HEX") == std::string::npos);
    REQUIRE(s.find("FAT12") == std::string::npos);
    REQUIRE(s.find("Extended BPB") == std::string::npos);
}

TEST_CASE("write_report lists FFS ADF", "[adf][report][ffs]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_ffs_image());
    REQUIRE(a.amiga.present);
    REQUIRE(a.amiga.ffs);
    dumpfloppy::report_options opt{};
    opt.color = false;
    std::ostringstream plain;
    dumpfloppy::write_report(a, plain, opt);
    const std::string s = plain.str();
    REQUIRE(s.find("AMIGA ADF / FFS") != std::string::npos);
    REQUIRE(s.find("FFS (Amiga)") != std::string::npos);
    REQUIRE(s.find("; not FAT") == std::string::npos);
    REQUIRE(s.find("FFSVOL") != std::string::npos);
    REQUIRE(s.find("RAWFILE") != std::string::npos);
    REQUIRE(s.find("BIOS PARAMETER BLOCK") == std::string::npos);
}

TEST_CASE("extract writes OFS file and skips directories", "[adf][extract]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_ofs_image());
    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "adf-out";
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);

    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 1);
    REQUIRE(err.str().empty());

    std::ifstream readme(dest / "README", std::ios::binary);
    REQUIRE(readme);
    const std::vector<uint8_t> body((std::istreambuf_iterator<char>(readme)),
                                    std::istreambuf_iterator<char>());
    REQUIRE(body == dumpfloppy_test::sample_ofs_payload());
}

TEST_CASE("extract flattens Amiga subdir path and skips DIR", "[adf][extract]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_subdir_image());
    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "adf-sub";
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);

    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 1);
    REQUIRE(err.str().empty());
    REQUIRE(std::filesystem::exists(dest / "Sub_Inner"));
    REQUIRE_FALSE(std::filesystem::exists(dest / "Sub"));

    std::ifstream inner(dest / "Sub_Inner", std::ios::binary);
    REQUIRE(inner);
    const std::vector<uint8_t> body((std::istreambuf_iterator<char>(inner)),
                                    std::istreambuf_iterator<char>());
    REQUIRE(body == dumpfloppy_test::sample_inner_payload());
}

TEST_CASE("extract glob matches Amiga path and host name", "[adf][extract]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_subdir_image());
    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "adf-glob";
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);

    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    opt.patterns.emplace_back("Sub_Inner");
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 1);
    REQUIRE(std::filesystem::exists(dest / "Sub_Inner"));
}

TEST_CASE("extract FFS payload matches raw 512-byte blocks", "[adf][extract][ffs]")
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(make_ffs_image());
    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "adf-ffs";
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);

    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 1);
    std::ifstream raw(dest / "RAWFILE", std::ios::binary);
    REQUIRE(raw);
    const std::vector<uint8_t> body((std::istreambuf_iterator<char>(raw)),
                                    std::istreambuf_iterator<char>());
    REQUIRE(body == dumpfloppy_test::sample_ffs_payload());
    REQUIRE(body[512] == 0xBBu);
}

TEST_CASE("update same-size OFS README in place", "[adf][update]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(make_ofs_image());
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto host = dir / "README";
    std::vector<uint8_t> neu = dumpfloppy_test::sample_ofs_payload();
    std::fill(neu.begin(), neu.end(), static_cast<uint8_t>(0x11));
    const char tag[] = "OFS-UPDATED";
    std::memcpy(neu.data(), tag, sizeof(tag) - 1u);
    {
        std::ofstream out(host, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out.write(reinterpret_cast<const char*>(neu.data()),
                  static_cast<std::streamsize>(neu.size()));
    }
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.push_back(host);
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == 1);
    REQUIRE(err.str().empty());
    dumpfloppy::amiga_file file{};
    for (const dumpfloppy::amiga_file& e : a.amiga.entries)
    {
        if (e.name == "README")
        {
            file = e;
        }
    }
    REQUIRE(dumpfloppy::read_amiga_file(a.image.bytes, a.amiga, file) == neu);
}

TEST_CASE("update same-size FFS file in place", "[adf][update]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(make_ffs_image());
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto host = dir / "RAWFILE";
    std::vector<uint8_t> neu = dumpfloppy_test::sample_ffs_payload();
    std::fill(neu.begin(), neu.end(), static_cast<uint8_t>(0x22));
    neu[512] = 0xDDu;
    {
        std::ofstream out(host, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out.write(reinterpret_cast<const char*>(neu.data()),
                  static_cast<std::streamsize>(neu.size()));
    }
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.push_back(host);
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == 1);
    REQUIRE(err.str().empty());
    dumpfloppy::amiga_file file{};
    for (const dumpfloppy::amiga_file& e : a.amiga.entries)
    {
        if (e.name == "RAWFILE")
        {
            file = e;
        }
    }
    REQUIRE(dumpfloppy::read_amiga_file(a.image.bytes, a.amiga, file) == neu);
}

TEST_CASE("update ADF refuses size mismatch", "[adf][update]")
{
    dumpfloppy::analysis a = dumpfloppy::analyse(make_ofs_image());
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto host = dir / "README";
    {
        std::ofstream out(host, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out << "short";
    }
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.push_back(host);
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == -1);
    REQUIRE(err.str().find("same-size") != std::string::npos);
}

TEST_CASE("load_image .adf is container adf_amiga", "[adf][image]")
{
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto path = dir / "sample.adf";
    const auto bytes = dumpfloppy_test::make_ofs_root_file_adf();
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded);
    REQUIRE(loaded->container == dumpfloppy::container_kind::adf_amiga);
    REQUIRE(loaded->size_geometry.media_name.find("Amiga") != std::string::npos);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.amiga.present);
    REQUIRE(a.amiga.volume_name == "TESTADF");
}

TEST_CASE("optional Beast_Sonix ADF listing via analyse", "[adf][optional][beast]")
{
    const std::filesystem::path img_path{
        "/mnt/music/_src/sotb/Beast_Sonix_1990_Scoopex.adf"};
    if (!std::filesystem::exists(img_path))
    {
        SKIP("Beast_Sonix ADF is not present");
    }
    auto loaded = dumpfloppy::load_image(img_path);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.amiga.present);
    REQUIRE_FALSE(a.cbm.present);
    REQUIRE_FALSE(a.amiga.ffs);
    REQUIRE_FALSE(a.bpb.looks_valid);
    REQUIRE(a.entries.empty());
    const bool name_ok = a.amiga.volume_name.find("Scoopex") != std::string::npos ||
                         a.amiga.volume_name.find("Beast") != std::string::npos;
    REQUIRE(name_ok);

    dumpfloppy::report_options opt{};
    opt.color = false;
    std::ostringstream plain;
    dumpfloppy::write_report(a, plain, opt);
    const std::string s = plain.str();
    REQUIRE(s.find("AMIGA ADF / OFS") != std::string::npos);
    const bool listed = s.find("Startup-Sequence") != std::string::npos ||
                        s.find("Shadow0") != std::string::npos;
    REQUIRE(listed);
}
