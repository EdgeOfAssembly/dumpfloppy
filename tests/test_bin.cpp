/**
 * @file test_bin.cpp
 * @brief Process-level CLI contracts against the dumpfloppy binary.
 */
#include "image_builder.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/wait.h>

namespace
{

std::string slurp_popen(const std::string& cmd, int& rc)
{
    rc = -1;
    FILE* fp = popen(cmd.c_str(), "r");
    if (fp == nullptr)
    {
        return {};
    }
    std::string out;
    char buf[4096];
    while (fgets(buf, sizeof(buf), fp) != nullptr)
    {
        out += buf;
    }
    rc = pclose(fp);
    if (rc != -1 && WIFEXITED(rc))
    {
        rc = WEXITSTATUS(rc);
    }
    return out;
}

const char* bin_or_skip()
{
    const char* b = std::getenv("DUMPFLOPPY_BIN");
    if (b == nullptr || b[0] == '\0')
    {
        SKIP("DUMPFLOPPY_BIN is not set");
    }
    return b;
}

} /* namespace */

TEST_CASE("binary -v prints dumpfloppy 0.8", "[cli][bin]")
{
    const char* bin = bin_or_skip();
    int rc = 0;
    const std::string out = slurp_popen(std::string(bin) + " -v", rc);
    REQUIRE(rc == 0);
    REQUIRE(out.find("dumpfloppy 0.8") != std::string::npos);
}

TEST_CASE("binary --version matches -v", "[cli][bin]")
{
    const char* bin = bin_or_skip();
    int rc1 = 0;
    int rc2 = 0;
    const std::string a = slurp_popen(std::string(bin) + " -v", rc1);
    const std::string b = slurp_popen(std::string(bin) + " --version", rc2);
    REQUIRE(rc1 == 0);
    REQUIRE(rc2 == 0);
    REQUIRE(a == b);
}

TEST_CASE("binary no-args and -h print usage", "[cli][bin]")
{
    const char* bin = bin_or_skip();
    int rc = 0;
    const std::string none = slurp_popen(std::string(bin), rc);
    REQUIRE(rc == 0);
    REQUIRE(none.find("Usage: dumpfloppy") != std::string::npos);
    int rc2 = 0;
    const std::string help = slurp_popen(std::string(bin) + " --help", rc2);
    REQUIRE(rc2 == 0);
    REQUIRE(help == none);
}

TEST_CASE("binary dumps a sample image with serial on stdout", "[cli][bin]")
{
    const char* bin = bin_or_skip();
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto img = dir / "cli-sample.ima";
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    {
        std::ofstream out(img, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }
    int rc = 0;
    const std::string cmd =
        std::string(bin) + " --no-color --no-hex \"" + img.string() + "\"";
    const std::string out = slurp_popen(cmd, rc);
    REQUIRE(rc == 0);
    REQUIRE(out.find("1234-ABCD") != std::string::npos);
    REQUIRE(out.find("HELLO.TXT") != std::string::npos);
    REQUIRE(out.find("deleted") != std::string::npos);
    REQUIRE(out.find("FAT12") != std::string::npos);
}

TEST_CASE("binary missing file is non-zero and message on stderr", "[cli][bin]")
{
    const char* bin = bin_or_skip();
    int rc = 0;
    const std::string cmd =
        std::string(bin) + " /no/such/dumpfloppy.ima 2>&1 >/dev/null";
    const std::string err = slurp_popen(cmd, rc);
    REQUIRE(rc != 0);
    REQUIRE(err.find("dumpfloppy:") != std::string::npos);
}

TEST_CASE("binary directory batch expands .ima files", "[cli][bin]")
{
    const char* bin = bin_or_skip();
    const auto dir =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "batch";
    std::filesystem::create_directories(dir);
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    for (const char* name : {"a.ima", "b.img"})
    {
        std::ofstream out(dir / name, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }
    int rc = 0;
    const std::string cmd =
        std::string(bin) + " --no-color --no-hex \"" + dir.string() + "\"";
    const std::string out = slurp_popen(cmd, rc);
    REQUIRE(rc == 0);
    /* Two reports. */
    size_t n = 0;
    for (size_t pos = 0; (pos = out.find("HELLO.TXT", pos)) != std::string::npos;
         pos += 9)
    {
        ++n;
    }
    REQUIRE(n >= 2);
}
