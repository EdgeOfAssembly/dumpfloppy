/**
 * @file test_deleted.cpp
 * @brief Occupancy-aware deleted dirent recovery (Star Control TACTICS).
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/util.hpp"
#include "image_builder.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

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

const dumpfloppy::dir_entry* find_83(const dumpfloppy::analysis& a,
                                     const std::string& name, bool deleted)
{
    for (const dumpfloppy::dir_entry& e : a.entries)
    {
        if (e.name_83 == name && e.deleted == deleted)
        {
            return &e;
        }
    }
    return nullptr;
}

std::string slurp(const std::filesystem::path& p)
{
    std::ifstream in(p, std::ios::binary);
    REQUIRE(in);
    return std::string((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
}

} /* namespace */

TEST_CASE("deleted TACTICS does not inherit the live FAT chain", "[deleted][tactics]")
{
    const auto bytes = dumpfloppy_test::make_fat12_tactics_same_cluster();
    const auto img = write_temp(bytes, "tactics-same.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));

    const dumpfloppy::dir_entry* live = find_83(a, "TACTICS.PKG", false);
    const dumpfloppy::dir_entry* dead = find_83(a, "?ACTICS.PKG", true);
    REQUIRE(live != nullptr);
    REQUIRE(dead != nullptr);
    REQUIRE(live->first_cluster == 2);
    REQUIRE(dead->first_cluster == 2);
    REQUIRE(live->size == dead->size);
    REQUIRE(live->size == 600);
    REQUIRE(live->cluster_chain.size() == 2);
    REQUIRE(dead->cluster_chain.empty());
    REQUIRE(dead->notes.find("reused") != std::string::npos);

    const std::vector<uint8_t> live_bytes = dumpfloppy_test::tactics_live_bytes();
    REQUIRE(dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *live) == live_bytes);
    const std::vector<uint8_t> dead_bytes =
        dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *dead);
    REQUIRE(dead_bytes.empty());
    REQUIRE(live->xxh64 == dumpfloppy::xxh64_hex(live_bytes));
    REQUIRE(dead->xxh64 == dumpfloppy::xxh64_hex(dead_bytes));
    REQUIRE(dead->xxh64 != live->xxh64);

    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "out-tactics";
    std::filesystem::remove_all(dest);
    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) >= 2);
    REQUIRE(slurp(dest / "TACTICS.PKG") ==
            std::string(live_bytes.begin(), live_bytes.end()));
    const std::string extracted_dead = slurp(dest / "?ACTICS.PKG");
    REQUIRE(extracted_dead.empty());
    REQUIRE(extracted_dead != std::string(live_bytes.begin(), live_bytes.end()));
}

TEST_CASE("deleted GONE.TXT still recovers its own allocated cluster",
          "[deleted][orphan]")
{
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    const auto img = write_temp(bytes, "gone-orphan.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));

    const dumpfloppy::dir_entry* gone = find_83(a, "?ONE.TXT", true);
    REQUIRE(gone != nullptr);
    REQUIRE(gone->first_cluster == 3);
    REQUIRE(gone->cluster_chain.size() == 1);
    REQUIRE(gone->cluster_chain[0] == 3);
    REQUIRE(dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *gone) ==
            (std::vector<uint8_t>{'B', 'Y', 'E', '\n'}));

    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "out-gone";
    std::filesystem::remove_all(dest);
    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    opt.patterns.emplace_back("?ONE.TXT");
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 1);
    REQUIRE(slurp(dest / "?ONE.TXT") == "BYE\n");
}

TEST_CASE("deleted contiguous recovery stops before a live-owned cluster",
          "[deleted][occupancy]")
{
    const auto bytes = dumpfloppy_test::make_fat12_deleted_before_live();
    const auto img = write_temp(bytes, "deleted-before-live.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));

    const dumpfloppy::dir_entry* dead = find_83(a, "?LD.BIN", true);
    const dumpfloppy::dir_entry* live = find_83(a, "NEW.BIN", false);
    REQUIRE(dead != nullptr);
    REQUIRE(live != nullptr);
    REQUIRE(dead->first_cluster == 2);
    REQUIRE(live->first_cluster == 3);
    REQUIRE(dead->cluster_chain.size() == 1);
    REQUIRE(dead->cluster_chain[0] == 2);
    REQUIRE(dead->notes.find("live-owned") != std::string::npos);

    const std::vector<uint8_t> recovered =
        dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *dead);
    REQUIRE(recovered.size() == 512);
    REQUIRE(recovered[0] == 'O');
    REQUIRE(std::string(recovered.begin(), recovered.begin() + 8) == "OLD-HEAD");
    const std::vector<uint8_t> live_bytes =
        dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *live);
    REQUIRE(live_bytes.size() == 512);
    REQUIRE(std::string(live_bytes.begin(), live_bytes.begin() + 13) == "NEW-LIVE-FILE");
    REQUIRE(recovered != live_bytes);
    REQUIRE(dead->xxh64 != live->xxh64);
}

TEST_CASE("tactics same-cluster image still extracts orphan GONE.TXT",
          "[deleted][tactics]")
{
    const auto bytes = dumpfloppy_test::make_fat12_tactics_same_cluster();
    const auto img = write_temp(bytes, "tactics-gone.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    const dumpfloppy::dir_entry* gone = find_83(a, "?ONE.TXT", true);
    REQUIRE(gone != nullptr);
    REQUIRE(gone->cluster_chain.size() == 1);
    REQUIRE(dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *gone) ==
            (std::vector<uint8_t>{'B', 'Y', 'E', '\n'}));
}
