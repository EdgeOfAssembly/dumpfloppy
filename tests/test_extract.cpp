/**
 * @file test_extract.cpp
 * @brief Glob matching, cluster-walk extract, MD5 / MIME identity.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/report.hpp"
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

} /* namespace */

TEST_CASE("glob_match understands * and ?", "[glob]")
{
    REQUIRE(dumpfloppy::glob_match("*.PKD", "591.PKD"));
    REQUIRE(dumpfloppy::glob_match("*.pkd", "591.PKD"));
    REQUIRE(dumpfloppy::glob_match("5??.PKD", "591.PKD"));
    REQUIRE_FALSE(dumpfloppy::glob_match("5??.PKD", "?91.PKD"));
    REQUIRE(dumpfloppy::glob_match("?91.PKD", "591.PKD"));
    REQUIRE(dumpfloppy::glob_match("?91.PKD", "?91.PKD"));
    REQUIRE(dumpfloppy::glob_match("*", "HELLO.TXT"));
    REQUIRE(dumpfloppy::glob_match("HELLO.TXT", "hello.txt"));
    REQUIRE_FALSE(dumpfloppy::glob_match("*.TXT", "HELLO.COM"));
    REQUIRE(dumpfloppy::glob_match("H*.TXT", "HELLO.TXT"));
}

TEST_CASE("extract writes payload and deleted files", "[extract]")
{
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    const auto img = write_temp(bytes, "extract.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));

    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "out";
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);

    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 2);

    std::ifstream hello(dest / "HELLO.TXT", std::ios::binary);
    REQUIRE(hello);
    std::string body((std::istreambuf_iterator<char>(hello)),
                     std::istreambuf_iterator<char>());
    REQUIRE(body == "Hello, floppy\n");

    std::ifstream gone(dest / "?ONE.TXT", std::ios::binary);
    REQUIRE(gone);
    std::string gone_body((std::istreambuf_iterator<char>(gone)),
                          std::istreambuf_iterator<char>());
    REQUIRE(gone_body == "BYE\n");
}

TEST_CASE("extract glob selects a subset", "[extract]")
{
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    const auto img = write_temp(bytes, "extract2.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));

    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "out2";
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);

    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    opt.patterns.emplace_back("HELLO.*");
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 1);
    REQUIRE(std::filesystem::exists(dest / "HELLO.TXT"));
    REQUIRE_FALSE(std::filesystem::exists(dest / "?ONE.TXT"));
}

TEST_CASE("analysis fills MD5 and MIME for payloads", "[extract][identity]")
{
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    const auto img = write_temp(bytes, "ident.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    bool saw = false;
    for (const dumpfloppy::dir_entry& e : a.entries)
    {
        if (e.name_83 == "HELLO.TXT")
        {
            saw = true;
            REQUIRE(e.md5 == "ec252e95cb88a8cb5c9682cd892a0ccf");
            REQUIRE(e.mime == "text/plain");
        }
    }
    REQUIRE(saw);
}

TEST_CASE("unmatched extract pattern fails", "[extract]")
{
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    const auto img = write_temp(bytes, "nomatch.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "empty";
    opt.patterns.emplace_back("NOPE.COM");
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == -1);
    REQUIRE(err.str().find("no files matched") != std::string::npos);
}
