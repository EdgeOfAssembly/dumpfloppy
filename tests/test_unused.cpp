/**
 * @file test_unused.cpp
 * @brief FAT-free/bad leftover cluster recovery (any FAT12/16 image).
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/cli.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/unused.hpp"
#include "image_builder.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

TEST_CASE("scan_unused_clusters skips empty and format-fill", "[unused]")
{
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    dumpfloppy::floppy_image img{};
    img.bytes = bytes;
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(img));
    REQUIRE(a.unused.empty());
}

TEST_CASE("unused leftover map and C source split into recovered files", "[unused]")
{
    const auto bytes = dumpfloppy_test::make_fat12_unused_leftover();
    dumpfloppy::floppy_image img{};
    img.bytes = bytes;
    img.path = "leftover.ima";
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(img));
    REQUIRE(a.unused.size() >= 2u);

    bool saw_map = false;
    bool saw_c = false;
    bool saw_bad = false;
    for (const dumpfloppy::unused_run& r : a.unused)
    {
        if (r.guess == "memory map")
        {
            saw_map = true;
            REQUIRE(r.host_name.find(".map") != std::string::npos);
            const std::string body(r.payload.begin(), r.payload.end());
            REQUIRE(body.find("LOADSEG") != std::string::npos);
            REQUIRE(body.find("9BA:1749") != std::string::npos);
        }
        if (r.guess == "C source")
        {
            saw_c = true;
            REQUIRE(r.host_name.find(".c") != std::string::npos);
            const std::string body(r.payload.begin(), r.payload.end());
            REQUIRE(body.find("#include") != std::string::npos);
        }
        if (r.kind == dumpfloppy::unused_kind::fat_bad)
        {
            saw_bad = true;
            REQUIRE(r.first_cluster == 16u);
        }
    }
    REQUIRE(saw_map);
    REQUIRE(saw_c);
    REQUIRE(saw_bad);

    bool saw_showobj = false;
    bool saw_display = false;
    for (const dumpfloppy::unused_run& r : a.unused)
    {
        if (r.host_name == "unused_SHOWOBJ.c")
        {
            saw_showobj = true;
            const std::string body(r.payload.begin(), r.payload.end());
            REQUIRE(body.find("ShowObj") != std::string::npos);
            REQUIRE(body.find("DisplayInput") == std::string::npos);
        }
        if (r.host_name == "unused_DISPLAY.c")
        {
            saw_display = true;
            const std::string body(r.payload.begin(), r.payload.end());
            REQUIRE(body.find("DisplayInput") != std::string::npos);
            REQUIRE(body.find("ShowObj") == std::string::npos);
        }
    }
    REQUIRE(saw_showobj);
    REQUIRE(saw_display);
}

TEST_CASE("write_report lists UNUSED leftover runs", "[unused][report]")
{
    dumpfloppy::floppy_image img{};
    img.bytes = dumpfloppy_test::make_fat12_unused_leftover();
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(img));
    dumpfloppy::report_options opt{};
    opt.color = false;
    opt.hex_boot = false;
    std::ostringstream plain;
    dumpfloppy::write_report(a, plain, opt);
    const std::string s = plain.str();
    REQUIRE(s.find("UNUSED") != std::string::npos);
    REQUIRE(s.find("memory map") != std::string::npos);
    REQUIRE(s.find("C source") != std::string::npos);
    REQUIRE(s.find("leftover data") != std::string::npos);

    opt.show_unused = false;
    std::ostringstream hidden;
    dumpfloppy::write_report(a, hidden, opt);
    REQUIRE(hidden.str().find("UNUSED") == std::string::npos);
}

TEST_CASE("extract writes unused leftover files", "[unused][extract]")
{
    dumpfloppy::floppy_image img{};
    img.bytes = dumpfloppy_test::make_fat12_unused_leftover();
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(img));
    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "unused-out";
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);
    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    const int n = dumpfloppy::extract_files(a, opt, err);
    REQUIRE(err.str().empty());
    REQUIRE(n >= 3); /* HELLO.TXT + map + C + bad leftover */

    bool found_c = false;
    bool found_map = false;
    for (const auto& ent : std::filesystem::directory_iterator(dest))
    {
        const std::string name = ent.path().filename().string();
        if (name.ends_with(".c"))
        {
            std::ifstream in(ent.path());
            const std::string body((std::istreambuf_iterator<char>(in)),
                                   std::istreambuf_iterator<char>());
            REQUIRE((body.find("DisplayInput") != std::string::npos ||
                     body.find("ShowObj") != std::string::npos));
            found_c = true;
        }
        if (name.ends_with(".map"))
        {
            std::ifstream in(ent.path());
            const std::string body((std::istreambuf_iterator<char>(in)),
                                   std::istreambuf_iterator<char>());
            REQUIRE(body.find("LOADSEG") != std::string::npos);
            found_map = true;
        }
    }
    REQUIRE(found_c);
    REQUIRE(found_map);
}

TEST_CASE("extract glob unused* selects leftover runs only", "[unused][extract]")
{
    dumpfloppy::floppy_image img{};
    img.bytes = dumpfloppy_test::make_fat12_unused_leftover();
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(img));
    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "unused-glob";
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);
    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    opt.patterns.emplace_back("unused*");
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) >= 2);
    REQUIRE_FALSE(std::filesystem::exists(dest / "HELLO.TXT"));
}

TEST_CASE("parse_cli --no-unused hides leftover listing only", "[unused][cli]")
{
    char a0[] = "dumpfloppy";
    char a1[] = "--no-unused";
    char a2[] = "x.ima";
    char* argv[] = {a0, a1, a2, nullptr};
    const dumpfloppy::cli_options o = dumpfloppy::parse_cli(3, argv);
    REQUIRE(o.ok);
    REQUIRE_FALSE(o.report.show_unused);
    REQUIRE(o.report.show_deleted);
}

TEST_CASE("SQ2 Disk 1 IMA recovers unused map and C source",
          "[unused][optional][sq2]")
{
    const std::filesystem::path img{
        "/tmp/Space Quest II - Chapter II - Vohaul's Revenge (1988) (v2.0F, Int. "
        "2.936) (Sierra On-Line, Inc.) (720K) [cp cr] [!]/Space Quest II - Chapter "
        "II - Vohaul's Revenge (1988) (v2.0F, Int. 2.936) (Sierra On-Line, Inc.) "
        "(720K) (Disk 1) [cr].ima"};
    const std::filesystem::path alt{
        "/mnt/dumpfloppy-fixtures/pc/sq2-disk1.ima"};
    const std::filesystem::path path = std::filesystem::exists(img) ? img : alt;
    if (!std::filesystem::exists(path))
    {
        SKIP("SQ2 Disk 1 .ima is not present");
    }
    auto loaded = dumpfloppy::load_image(path);
    REQUIRE(loaded);
    const dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    REQUIRE_FALSE(a.unused.empty());
    bool saw_map = false;
    bool saw_c = false;
    for (const dumpfloppy::unused_run& r : a.unused)
    {
        const std::string body(r.payload.begin(), r.payload.end());
        if (body.find("LOADSEG") != std::string::npos &&
            body.find("LOADVIEW") != std::string::npos)
        {
            saw_map = true;
        }
        if (body.find("DisplayInput") != std::string::npos &&
            body.find("#include") != std::string::npos)
        {
            saw_c = true;
        }
    }
    REQUIRE(saw_map);
    REQUIRE(saw_c);

    unsigned c_files = 0;
    bool saw_showobj = false;
    bool saw_animate = false;
    for (const dumpfloppy::unused_run& r : a.unused)
    {
        if (r.guess == "C source")
        {
            ++c_files;
        }
        if (r.host_name.find("SHOWOBJ") != std::string::npos)
        {
            saw_showobj = true;
        }
        if (r.host_name.find("ADVANCEL") != std::string::npos)
        {
            saw_animate = true;
        }
    }
    REQUIRE(c_files >= 10u);
    REQUIRE(saw_showobj);
    REQUIRE(saw_animate);
}
