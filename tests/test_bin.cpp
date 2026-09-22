/**
 * @file test_bin.cpp
 * @brief Process-level CLI contracts against the dumpfloppy binary.
 */
#include "dumpfloppy/version.hpp"
#include "image_builder.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
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

TEST_CASE("binary -v prints dumpfloppy k_version", "[cli][bin]")
{
    const char* bin = bin_or_skip();
    int rc = 0;
    const std::string out = slurp_popen(std::string(bin) + " -v", rc);
    REQUIRE(rc == 0);
    const std::string needle =
        std::string(dumpfloppy::k_program) + " " + dumpfloppy::k_version;
    REQUIRE(out.find(needle) != std::string::npos);
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

TEST_CASE("binary -x extracts without listing", "[cli][bin][extract]")
{
    const char* bin = bin_or_skip();
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests" /
                     "extract-silent";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    const auto img = dir / "sample.ima";
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    {
        std::ofstream out(img, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }
    int rc = 0;
    const std::string cmd = std::string("cd \"") + dir.string() + "\" && " + bin +
                            " -x HELLO.TXT \"" + img.string() + "\"";
    const std::string out = slurp_popen(cmd, rc);
    REQUIRE(rc == 0);
    REQUIRE(out.find("DIRECTORY") == std::string::npos);
    REQUIRE(out.find("IMAGE") == std::string::npos);
    REQUIRE(out.find("extracted") == std::string::npos);
    std::ifstream hello(dir / "HELLO.TXT", std::ios::binary);
    REQUIRE(hello);
    std::string body((std::istreambuf_iterator<char>(hello)),
                     std::istreambuf_iterator<char>());
    REQUIRE(body == "Hello, floppy\n");
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

TEST_CASE("binary glued -uFILE is not an unknown option", "[cli][bin]")
{
    const char* bin = bin_or_skip();
    int rc = 0;
    const std::string err =
        slurp_popen(std::string(bin) + " -uHELLO.TXT 2>&1 >/dev/null", rc);
    REQUIRE(err.find("unknown option") == std::string::npos);
    REQUIRE(rc != 0);
}

TEST_CASE("binary -u with no image exits 1, not usage", "[cli][bin][update]")
{
    const char* bin = bin_or_skip();
    int rc = 0;
    const std::string out =
        slurp_popen(std::string(bin) + " -u HELLO.TXT 2>&1", rc);
    REQUIRE(rc != 0);
    REQUIRE(out.find("Usage:") == std::string::npos);
    REQUIRE(out.find("dumpfloppy:") != std::string::npos);
    REQUIRE(out.find("no image") != std::string::npos);
}

TEST_CASE("binary -x with no image exits 1, not usage", "[cli][bin][extract]")
{
    const char* bin = bin_or_skip();
    int rc = 0;
    const std::string out = slurp_popen(std::string(bin) + " -x 2>&1", rc);
    REQUIRE(rc != 0);
    REQUIRE(out.find("Usage:") == std::string::npos);
    REQUIRE(out.find("dumpfloppy:") != std::string::npos);
    REQUIRE(out.find("no image") != std::string::npos);
}

TEST_CASE("binary -o with -x warns that -o is ignored", "[cli][bin][extract]")
{
    const char* bin = bin_or_skip();
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests" /
                     "o-ignored-x";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    const auto img = dir / "sample.ima";
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    {
        std::ofstream out(img, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }
    int rc = 0;
    const std::string cmd = std::string("cd \"") + dir.string() + "\" && " + bin +
                            " -o ignored.txt -x HELLO.TXT \"" + img.string() +
                            "\" 2>&1";
    const std::string out = slurp_popen(cmd, rc);
    REQUIRE(rc == 0);
    REQUIRE(out.find("ignored") != std::string::npos);
    REQUIRE(out.find("-o") != std::string::npos);
    REQUIRE_FALSE(std::filesystem::exists(dir / "ignored.txt"));
    std::ifstream hello(dir / "HELLO.TXT", std::ios::binary);
    REQUIRE(hello);
}

TEST_CASE("binary glued -uFILE overwrites silently and round-trips",
          "[cli][bin][update]")
{
    const char* bin = bin_or_skip();
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests" /
                     "update-glued";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    const auto img = dir / "sample.ima";
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    {
        std::ofstream out(img, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }
    const auto host = dir / "HELLO.TXT";
    {
        std::ofstream out(host, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out << "Glued payload!\nY";
    }
    int rc = 0;
    const std::string cmd = std::string("cd \"") + dir.string() + "\" && " + bin +
                            " -uHELLO.TXT \"" + img.string() + "\"";
    const std::string out = slurp_popen(cmd, rc);
    REQUIRE(rc == 0);
    REQUIRE(out.empty());

    const auto xdir = dir / "x";
    std::filesystem::create_directories(xdir);
    int rc2 = 0;
    const std::string xcmd = std::string("cd \"") + xdir.string() + "\" && " + bin +
                             " -x HELLO.TXT \"" + img.string() + "\"";
    const std::string xout = slurp_popen(xcmd, rc2);
    REQUIRE(rc2 == 0);
    REQUIRE(xout.empty());
    std::ifstream hello(xdir / "HELLO.TXT", std::ios::binary);
    REQUIRE(hello);
    std::string body((std::istreambuf_iterator<char>(hello)),
                     std::istreambuf_iterator<char>());
    REQUIRE(body == "Glued payload!\nY");
}

TEST_CASE("binary -u overwrites silently and round-trips", "[cli][bin][update]")
{
    const char* bin = bin_or_skip();
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests" /
                     "update-silent";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    const auto img = dir / "sample.ima";
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    {
        std::ofstream out(img, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }
    const auto host = dir / "HELLO.TXT";
    {
        std::ofstream out(host, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out << "New payload!\nX";
    }
    int rc = 0;
    const std::string cmd = std::string("cd \"") + dir.string() + "\" && " + bin +
                            " -u HELLO.TXT \"" + img.string() + "\"";
    const std::string out = slurp_popen(cmd, rc);
    REQUIRE(rc == 0);
    REQUIRE(out.empty());

    const auto xdir = dir / "x";
    std::filesystem::create_directories(xdir);
    int rc2 = 0;
    const std::string xcmd = std::string("cd \"") + xdir.string() + "\" && " + bin +
                             " -x HELLO.TXT \"" + img.string() + "\"";
    const std::string xout = slurp_popen(xcmd, rc2);
    REQUIRE(rc2 == 0);
    REQUIRE(xout.empty());
    std::ifstream hello(xdir / "HELLO.TXT", std::ios::binary);
    REQUIRE(hello);
    std::string body((std::istreambuf_iterator<char>(hello)),
                     std::istreambuf_iterator<char>());
    REQUIRE(body == "New payload!\nX");
    REQUIRE_FALSE(std::filesystem::exists(
        std::filesystem::path(img.string() + ".dumpfloppy-tmp")));
}
