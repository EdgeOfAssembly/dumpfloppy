/**
 * @file test_extract_dest.cpp
 * @brief `-x` honours `-o`, extracted names are printable, `--offset` maps FAT.
 */
#include "dumpfloppy/cli.hpp"
#include "image_builder.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <sys/wait.h>
#include <vector>

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
    const int closed = pclose(fp);
    if (closed == -1)
    {
        rc = -1;
        return out;
    }
    if (WIFEXITED(closed))
    {
        rc = WEXITSTATUS(closed);
    }
    else
    {
        rc = closed;
    }
    return out;
}

const char* bin_or_require()
{
    const char* bin = std::getenv("DUMPFLOPPY_BIN");
    REQUIRE(bin != nullptr);
    REQUIRE(bin[0] != '\0');
    return bin;
}

dumpfloppy::cli_options parse(const std::vector<std::string>& args)
{
    std::vector<std::string> storage;
    storage.reserve(args.size() + 1u);
    storage.emplace_back("dumpfloppy");
    storage.insert(storage.end(), args.begin(), args.end());
    std::vector<char*> ptrs;
    ptrs.reserve(storage.size());
    for (std::string& s : storage)
    {
        ptrs.push_back(s.data());
    }
    return dumpfloppy::parse_cli(static_cast<int>(ptrs.size()), ptrs.data());
}

std::filesystem::path scratch_root(const char* name)
{
    const auto dir =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / name;
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir;
}

void write_bytes(const std::filesystem::path& path, const std::vector<uint8_t>& bytes)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    REQUIRE(out);
    if (!bytes.empty())
    {
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }
    REQUIRE(out);
}

bool name_is_printable(const std::string& name)
{
    if (name.empty())
    {
        return false;
    }
    for (const unsigned char c : name)
    {
        if (c < 0x20u || c > 0x7Eu || c == 0xFFu)
        {
            return false;
        }
    }
    return true;
}

} /* namespace */

TEST_CASE("parse_cli --offset is enable-only and interleaved", "[cli][offset]")
{
    const auto dec = parse({"disk.ima", "--offset", "2048"});
    REQUIRE(dec.ok);
    REQUIRE(dec.has_offset);
    REQUIRE(dec.offset == 2048u);
    REQUIRE(dec.inputs.size() == 1u);
    REQUIRE(dec.inputs[0] == "disk.ima");

    const auto hex = parse({"--offset=0x800", "--no-hex", "disk.ima"});
    REQUIRE(hex.ok);
    REQUIRE(hex.has_offset);
    REQUIRE(hex.offset == 2048u);
    REQUIRE_FALSE(hex.report.hex_boot);
    REQUIRE(hex.inputs[0] == "disk.ima");

    const auto zero = parse({"--offset=0", "a.img"});
    REQUIRE(zero.ok);
    REQUIRE(zero.has_offset);
    REQUIRE(zero.offset == 0u);

    const auto bad = parse({"--offset=zz", "a.img"});
    REQUIRE_FALSE(bad.ok);
    REQUIRE(bad.error.find("invalid offset") != std::string::npos);

    const auto missing = parse({"--offset"});
    REQUIRE_FALSE(missing.ok);
    REQUIRE(missing.error.find("missing N") != std::string::npos);

    const std::string usage = dumpfloppy::usage_text();
    REQUIRE(usage.find("--offset") != std::string::npos);
    REQUIRE(usage.find("--no-offset") == std::string::npos);
    REQUIRE(usage.find("-o DIR") != std::string::npos);
}

TEST_CASE("-x -o writes into that directory and not the cwd", "[cli][bin][extract]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("extract-dest");
    const auto img = dir / "sample.ima";
    write_bytes(img, dumpfloppy_test::make_fat12_sample());
    const auto dest = dir / "out";
    const auto repo_hello = std::filesystem::current_path() / "HELLO.TXT";
    const bool repo_had_hello = std::filesystem::exists(repo_hello);

    int rc = 0;
    const std::string cmd = std::string("cd \"") + dir.string() + "\" && \"" + bin +
                            "\" -x -o \"" + dest.string() + "\" \"" + img.string() +
                            "\" 2>&1";
    const std::string out = slurp_popen(cmd, rc);
    REQUIRE(rc == 0);
    REQUIRE(out.find("ignored") == std::string::npos);
    REQUIRE(std::filesystem::is_regular_file(dest / "HELLO.TXT"));
    REQUIRE(std::filesystem::is_regular_file(dest / "?ONE.TXT"));
    REQUIRE_FALSE(std::filesystem::exists(dir / "HELLO.TXT"));
    REQUIRE_FALSE(std::filesystem::exists(dir / "?ONE.TXT"));
    if (!repo_had_hello)
    {
        REQUIRE_FALSE(std::filesystem::exists(repo_hello));
    }
}

TEST_CASE("-x -o existing file fails and does not extract into cwd",
          "[cli][bin][extract]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("extract-dest-file");
    const auto img = dir / "sample.ima";
    write_bytes(img, dumpfloppy_test::make_fat12_sample());
    const auto blocker = dir / "not-a-dir";
    {
        std::ofstream out(blocker, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out << "keep\n";
    }

    int rc = 0;
    const std::string cmd = std::string("cd \"") + dir.string() + "\" && \"" + bin +
                            "\" -o \"" + blocker.string() + "\" -x \"" + img.string() +
                            "\" 2>&1";
    const std::string out = slurp_popen(cmd, rc);
    REQUIRE(rc != 0);
    REQUIRE(out.find("existing file") != std::string::npos);
    REQUIRE_FALSE(std::filesystem::exists(dir / "HELLO.TXT"));
    REQUIRE_FALSE(std::filesystem::exists(dir / "?ONE.TXT"));
    REQUIRE(std::filesystem::is_regular_file(blocker));
    std::ifstream kept(blocker);
    std::string body;
    std::getline(kept, body);
    REQUIRE(body == "keep");
}

TEST_CASE("-u without -x still ignores -o", "[cli][bin][extract]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("extract-u-ignore-o");
    const auto img = dir / "sample.ima";
    write_bytes(img, dumpfloppy_test::make_fat12_sample());
    const auto ignored = dir / "ignored.txt";

    int rc = 0;
    const std::string cmd = std::string("cd \"") + dir.string() + "\" && \"" + bin +
                            "\" -u HELLO.TXT -o \"" + ignored.string() + "\" \"" +
                            img.string() + "\" 2>&1";
    const std::string out = slurp_popen(cmd, rc);
    REQUIRE(rc != 0);
    REQUIRE(out.find("ignored") != std::string::npos);
    REQUIRE(out.find("-o") != std::string::npos);
    REQUIRE_FALSE(std::filesystem::exists(ignored));
    REQUIRE_FALSE(std::filesystem::exists(dir / "HELLO.TXT"));
}

TEST_CASE("-u -x -o uses the extract directory and does not ignore -o",
          "[cli][bin][extract]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("extract-uxo");
    const auto img = dir / "sample.ima";
    write_bytes(img, dumpfloppy_test::make_fat12_sample());
    {
        std::ofstream host(dir / "HELLO.TXT", std::ios::binary | std::ios::trunc);
        REQUIRE(host);
        host << "Hello, floppy!";
    }
    const auto dest = dir / "out";

    int rc = 0;
    const std::string cmd = std::string("cd \"") + dir.string() + "\" && \"" + bin +
                            "\" -u HELLO.TXT -x -o \"" + dest.string() + "\" \"" +
                            img.string() + "\" 2>&1";
    const std::string out = slurp_popen(cmd, rc);
    REQUIRE(rc == 0);
    REQUIRE(out.find("ignored") == std::string::npos);
    std::ifstream extracted(dest / "HELLO.TXT", std::ios::binary);
    REQUIRE(extracted);
    std::string body((std::istreambuf_iterator<char>(extracted)),
                     std::istreambuf_iterator<char>());
    REQUIRE(body == "Hello, floppy!");
    REQUIRE_FALSE(std::filesystem::exists(dir / "?ONE.TXT"));
}

TEST_CASE("extract replaces 0xFF in a FAT 8.3 name", "[cli][bin][extract]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("extract-ff-name");
    auto bytes = dumpfloppy_test::make_fat12_sample();
    uint8_t* hello = dumpfloppy_test::fat12_root(bytes) + 32;
    hello[0] = 0xFFu;
    hello[1] = 0xFFu;
    for (int i = 2; i < 8; ++i)
    {
        hello[i] = static_cast<uint8_t>(' ');
    }
    const auto img = dir / "ff.ima";
    write_bytes(img, bytes);
    const auto dest = dir / "out";

    int rc = 0;
    const std::string cmd = std::string("cd \"") + dir.string() + "\" && \"" + bin +
                            "\" -x -o \"" + dest.string() + "\" \"" + img.string() +
                            "\" 2>&1";
    const std::string out = slurp_popen(cmd, rc);
    REQUIRE(rc == 0);
    REQUIRE(out.find("unsafe") == std::string::npos);
    REQUIRE(std::filesystem::is_directory(dest));

    bool saw_sanitized = false;
    for (const auto& ent : std::filesystem::recursive_directory_iterator(dest))
    {
        const std::string name = ent.path().filename().string();
        REQUIRE(name_is_printable(name));
        if (name == "__.TXT")
        {
            saw_sanitized = true;
        }
    }
    REQUIRE(saw_sanitized);
    REQUIRE(std::filesystem::is_regular_file(dest / "?ONE.TXT"));
    REQUIRE_FALSE(std::filesystem::exists(dir / "__.TXT"));
}

TEST_CASE("--offset maps the boot sector and HELLO.TXT on a FAT12 sample",
          "[cli][bin][offset]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("offset-fat12");
    const auto img = dir / "sample.ima";
    write_bytes(img, dumpfloppy_test::make_fat12_sample());
    const auto payload = dumpfloppy_test::fat12_data_off();
    REQUIRE(payload == 2048u);

    int rc0 = 0;
    const std::string boot = slurp_popen(std::string("\"") + bin + "\" --offset=0 \"" +
                                              img.string() + "\" 2>&1",
                                          rc0);
    REQUIRE(rc0 == 0);
    REQUIRE(boot.find("0 sector 0 reserved/FAT/root") != std::string::npos);
    REQUIRE(boot.find("cluster") == std::string::npos);
    REQUIRE(boot.find("HELLO") == std::string::npos);

    int rc1 = 0;
    const std::string file =
        slurp_popen(std::string("\"") + bin + "\" \"" + img.string() +
                        "\" --offset=2048 2>&1",
                    rc1);
    REQUIRE(rc1 == 0);
    REQUIRE(file.find("2048 sector 4 cluster 2 HELLO.TXT") != std::string::npos);

    int rc2 = 0;
    const std::string hex =
        slurp_popen(std::string("\"") + bin + "\" --offset=0x800 \"" + img.string() +
                        "\"",
                    rc2);
    REQUIRE(rc2 == 0);
    REQUIRE(hex.find("2048 sector 4 cluster 2 HELLO.TXT") != std::string::npos);

    int rc3 = 0;
    const std::string slack =
        slurp_popen(std::string("\"") + bin + "\" --offset=" +
                        std::to_string(payload + 14u) + " \"" + img.string() + "\"",
                    rc3);
    REQUIRE(rc3 == 0);
    REQUIRE(slack.find("slack") != std::string::npos);
    REQUIRE(slack.find("cluster 2") != std::string::npos);
    REQUIRE(slack.find("HELLO") == std::string::npos);
}

TEST_CASE("--offset rejects a non-FAT file", "[cli][bin][offset]")
{
    const char* bin = bin_or_require();
    const auto dir = scratch_root("offset-not-fat");
    const auto txt = dir / "notes.txt";
    {
        std::ofstream out(txt, std::ios::binary | std::ios::trunc);
        REQUIRE(out);
        out << "not a floppy\n";
    }

    int rc = 0;
    const std::string out = slurp_popen(std::string("\"") + bin + "\" --offset=0 \"" +
                                             txt.string() + "\" 2>&1",
                                         rc);
    REQUIRE(rc != 0);
    REQUIRE(out.find("offset map is only implemented for FAT12/FAT16") !=
            std::string::npos);
    REQUIRE(out.find("cluster") == std::string::npos);
    REQUIRE(out.find("HELLO") == std::string::npos);
}

TEST_CASE("-h mentions --offset", "[cli][bin][offset]")
{
    const char* bin = bin_or_require();
    int rc = 0;
    const std::string out = slurp_popen(std::string("\"") + bin + "\" -h", rc);
    REQUIRE(rc == 0);
    REQUIRE(out.find("--offset") != std::string::npos);
    REQUIRE(out.find("--no-offset") == std::string::npos);
}
