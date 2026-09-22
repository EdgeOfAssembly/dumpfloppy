/**
 * @file test_extract.cpp
 * @brief Glob matching, cluster-walk extract, XXH64 identity.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/types.hpp"
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
    REQUIRE(err.str().empty());

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

TEST_CASE("analysis fills XXH64 and Type DATA for payloads", "[extract][identity]")
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
            REQUIRE(e.xxh64 == "a41fb567443800ac");
            REQUIRE(e.type == "DATA");
        }
        if (e.name_83 == "TESTVOL")
        {
            REQUIRE(e.type == "VOL");
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

TEST_CASE("extract rejects absolute LFN paths", "[extract][safety]")
{
    dumpfloppy::analysis a{};
    dumpfloppy::dir_entry abs_etc{};
    abs_etc.path = "/etc/passwd";
    abs_etc.name_83 = "PASSWD";
    abs_etc.lfn = "/etc/passwd";
    abs_etc.attributes = dumpfloppy::k_attr_archive;
    a.entries.push_back(abs_etc);

    dumpfloppy::dir_entry abs_tmp{};
    abs_tmp.path = "/tmp/dumpfloppy-extract-escape-probe";
    abs_tmp.name_83 = "PROBE";
    abs_tmp.lfn = "/tmp/dumpfloppy-extract-escape-probe";
    abs_tmp.attributes = dumpfloppy::k_attr_archive;
    a.entries.push_back(abs_tmp);

    dumpfloppy::dir_entry win_abs{};
    win_abs.path = "\\tmp\\x";
    win_abs.name_83 = "X.TXT";
    win_abs.attributes = dumpfloppy::k_attr_archive;
    a.entries.push_back(win_abs);

    dumpfloppy::dir_entry dots{};
    dots.path = "..\\..\\etc\\passwd";
    dots.name_83 = "PASSWD";
    dots.attributes = dumpfloppy::k_attr_archive;
    a.entries.push_back(dots);

    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "out-abs";
    std::filesystem::remove_all(dest);
    const std::filesystem::path probe{"/tmp/dumpfloppy-extract-escape-probe"};
    std::filesystem::remove(probe);

    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 0);
    REQUIRE(err.str().find("unsafe") != std::string::npos);
    REQUIRE_FALSE(std::filesystem::exists(probe));
    REQUIRE_FALSE(std::filesystem::exists(dest / "etc" / "passwd"));
    REQUIRE_FALSE(std::filesystem::exists(dest / "passwd"));
    REQUIRE_FALSE(std::filesystem::exists(dest / "x"));
    REQUIRE_FALSE(std::filesystem::exists(dest / "PROBE"));
}

TEST_CASE("extract collision keeps both payloads and warns", "[extract][collision]")
{
    dumpfloppy::analysis a{};
    dumpfloppy::dir_entry live{};
    live.path = "TACTICS.PKG";
    live.name_83 = "TACTICS.PKG";
    live.lfn = "TACTICS.PKG";
    live.attributes = dumpfloppy::k_attr_archive;
    live.deleted = false;
    a.entries.push_back(live);

    dumpfloppy::dir_entry dead{};
    dead.path = "TACTICS.PKG";
    dead.name_83 = "?ACTICS.PKG";
    dead.lfn = "TACTICS.PKG";
    dead.attributes = dumpfloppy::k_attr_archive;
    dead.deleted = true;
    a.entries.push_back(dead);

    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "out-col";
    std::filesystem::remove_all(dest);

    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 2);
    REQUIRE(err.str().find("collision") != std::string::npos);
    REQUIRE(std::filesystem::exists(dest / "TACTICS.PKG"));
    REQUIRE(std::filesystem::exists(dest / "?ACTICS.PKG"));
}

TEST_CASE("extract collision uses stem.deleted.ext when 8.3 also collides",
          "[extract][collision]")
{
    dumpfloppy::analysis a{};
    dumpfloppy::dir_entry first{};
    first.path = "FOO.TXT";
    first.name_83 = "FOO.TXT";
    first.attributes = dumpfloppy::k_attr_archive;
    a.entries.push_back(first);

    dumpfloppy::dir_entry second{};
    second.path = "FOO.TXT";
    second.name_83 = "FOO.TXT";
    second.attributes = dumpfloppy::k_attr_archive;
    a.entries.push_back(second);

    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "out-col2";
    std::filesystem::remove_all(dest);

    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 2);
    REQUIRE(err.str().find("collision") != std::string::npos);
    REQUIRE(std::filesystem::exists(dest / "FOO.TXT"));
    REQUIRE(std::filesystem::exists(dest / "FOO.deleted.TXT"));
}

TEST_CASE("extract refuses 86BOX 86F flux images", "[extract][86f]")
{
    const std::vector<uint8_t> bytes{'8', '6', 'B', 'F', 12, 2, 0, 0};
    const auto img = write_temp(bytes, "tiny.86f");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE(a.flux.format_name == "86BOX 86F");

    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "out-86f";
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == -1);
    REQUIRE(err.str().find(".86f") != std::string::npos);
    REQUIRE(err.str().find(".mfm") != std::string::npos);
}

TEST_CASE("extract PENGUIN.EXE from Batman MFM CHS", "[extract][batman]")
{
    const std::filesystem::path img{
        "/tmp/Batman - The Caped Crusader (1989) (Data East USA, Inc.) (360K) [cp] [!]/"
        "Batman - The Caped Crusader (1989) (Data East USA, Inc.) (360K) (Disk 1) [cp] [!].mfm"};
    if (!std::filesystem::exists(img))
    {
        SKIP("Batman Disk 1 .mfm is not present");
    }
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "batman-x";
    std::filesystem::remove_all(dest);
    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    opt.patterns.emplace_back("PENGUIN.EXE");
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 1);
    REQUIRE(err.str().empty());
    std::ifstream exe(dest / "PENGUIN.EXE", std::ios::binary);
    REQUIRE(exe);
    char mz[2] = {};
    exe.read(mz, 2);
    REQUIRE(mz[0] == 'M');
    REQUIRE(mz[1] == 'Z');
}
