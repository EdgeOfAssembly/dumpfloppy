/**
 * @file test_cli.cpp
 * @brief Help/version/no-args/order-independence contracts.
 */
#include "dumpfloppy/cli.hpp"
#include "dumpfloppy/version.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{

dumpfloppy::cli_options parse(const std::vector<std::string>& args)
{
    std::vector<char*> ptrs;
    std::vector<std::string> storage;
    storage.push_back("dumpfloppy");
    storage.insert(storage.end(), args.begin(), args.end());
    ptrs.reserve(storage.size());
    for (std::string& s : storage)
    {
        ptrs.push_back(s.data());
    }
    return dumpfloppy::parse_cli(static_cast<int>(ptrs.size()), ptrs.data());
}

} /* namespace */

TEST_CASE("usage text names the program and the core flags", "[cli]")
{
    const std::string u = dumpfloppy::usage_text();
    REQUIRE(u.find("Usage: dumpfloppy") != std::string::npos);
    REQUIRE(u.find("-h, --help") != std::string::npos);
    REQUIRE(u.find("-v, --version") != std::string::npos);
    REQUIRE(u.find("--no-color") != std::string::npos);
    REQUIRE(u.find("-o, --output") != std::string::npos);
    REQUIRE(u.find("-x, --extract") != std::string::npos);
    REQUIRE(u.find("-u, --update") != std::string::npos);
    REQUIRE(u.find("-uFILE") != std::string::npos);
    REQUIRE(u.find("listing only") != std::string::npos);
    REQUIRE(u.find(".mfm") != std::string::npos);
    REQUIRE(u.find(".86f") != std::string::npos);
    REQUIRE(u.find(".d64") != std::string::npos);
    REQUIRE(u.find(".d71") != std::string::npos);
    REQUIRE(u.find(".d81") != std::string::npos);
    REQUIRE(u.find(".adf") != std::string::npos);
    REQUIRE(u.find(dumpfloppy::k_version) != std::string::npos);
}

TEST_CASE("parse_cli accepts interleaved options and operands", "[cli]")
{
    const auto a = parse({"disk.ima", "--no-color", "-o", "out.txt"});
    REQUIRE(a.ok);
    REQUIRE_FALSE(a.report.color);
    REQUIRE(a.has_output);
    REQUIRE(a.output == "out.txt");
    REQUIRE(a.inputs.size() == 1);
    REQUIRE(a.inputs[0] == "disk.ima");

    const auto b = parse({"-o", "out.txt", "--no-color", "disk.ima"});
    REQUIRE(b.ok);
    REQUIRE(b.inputs[0] == "disk.ima");
    REQUIRE(b.output == "out.txt");
}

TEST_CASE("parse_cli help and version flags", "[cli]")
{
    REQUIRE(parse({"-h"}).help);
    REQUIRE(parse({"--help"}).help);
    REQUIRE(parse({"-v"}).version);
    REQUIRE(parse({"--version"}).version);
}

TEST_CASE("parse_cli unknown option fails", "[cli]")
{
    const auto o = parse({"--verbose", "x.img"});
    REQUIRE_FALSE(o.ok);
    REQUIRE(o.error.find("unknown") != std::string::npos);
}

TEST_CASE("parse_cli --no-hex and --no-deleted", "[cli]")
{
    const auto o = parse({"--no-hex", "--no-deleted", "a.img"});
    REQUIRE(o.ok);
    REQUIRE_FALSE(o.report.hex_boot);
    REQUIRE_FALSE(o.report.show_deleted);
}

TEST_CASE("parse_cli -x extract all vs glob vs --extract=", "[cli]")
{
    const auto all = parse({"disk.ima", "-x"});
    REQUIRE(all.ok);
    REQUIRE(all.extract.enabled);
    REQUIRE(all.extract.patterns.empty());
    REQUIRE(all.inputs.size() == 1);

    const auto glob = parse({"disk.ima", "-x", "*.PKD"});
    REQUIRE(glob.ok);
    REQUIRE(glob.extract.enabled);
    REQUIRE(glob.extract.patterns.size() == 1);
    REQUIRE(glob.extract.patterns[0] == "*.PKD");
    REQUIRE(glob.inputs.size() == 1);

    const auto eq = parse({"--extract=5??.PKD", "disk.ima"});
    REQUIRE(eq.ok);
    REQUIRE(eq.extract.patterns[0] == "5??.PKD");

    /* .ima / .d64 suffix is an image operand, not a glob. */
    const auto img = parse({"-x", "disk.ima"});
    REQUIRE(img.ok);
    REQUIRE(img.extract.enabled);
    REQUIRE(img.extract.patterns.empty());
    REQUIRE(img.inputs.size() == 1);
    REQUIRE(img.inputs[0] == "disk.ima");

    const auto d64 = parse({"-x", "game.d64"});
    REQUIRE(d64.ok);
    REQUIRE(d64.extract.enabled);
    REQUIRE(d64.extract.patterns.empty());
    REQUIRE(d64.inputs.size() == 1);
    REQUIRE(d64.inputs[0] == "game.d64");

    const auto d71 = parse({"-x", "disk.d71"});
    REQUIRE(d71.ok);
    REQUIRE(d71.extract.patterns.empty());
    REQUIRE(d71.inputs[0] == "disk.d71");

    const auto d81 = parse({"-x", "disk.d81"});
    REQUIRE(d81.ok);
    REQUIRE(d81.extract.patterns.empty());
    REQUIRE(d81.inputs[0] == "disk.d81");

    const auto adf = parse({"-x", "work.adf"});
    REQUIRE(adf.ok);
    REQUIRE(adf.extract.patterns.empty());
    REQUIRE(adf.inputs[0] == "work.adf");

    const auto named = parse({"disk.ima", "-x", "591.PKD"});
    REQUIRE(named.extract.patterns[0] == "591.PKD");
}

TEST_CASE("parse_cli glued -uFILE like -xGLOB", "[cli]")
{
    const auto glued = parse({"-uPENGUIN.EXE", "disk.mfm"});
    REQUIRE(glued.ok);
    REQUIRE(glued.update.enabled);
    REQUIRE(glued.update.hosts.size() == 1);
    REQUIRE(glued.update.hosts[0] == "PENGUIN.EXE");
    REQUIRE(glued.inputs.size() == 1);
    REQUIRE(glued.inputs[0] == "disk.mfm");

    const auto spaced = parse({"disk.ima", "-u", "HELLO.TXT"});
    REQUIRE(spaced.ok);
    REQUIRE(spaced.update.enabled);
    REQUIRE(spaced.update.hosts.size() == 1);
    REQUIRE(spaced.update.hosts[0] == "HELLO.TXT");

    const auto eq = parse({"--update=FILEB.TXT", "disk.ima"});
    REQUIRE(eq.ok);
    REQUIRE(eq.update.hosts[0] == "FILEB.TXT");

    const auto many = parse({"-uA.TXT", "-uB.TXT", "disk.ima"});
    REQUIRE(many.ok);
    REQUIRE(many.update.hosts.size() == 2);
    REQUIRE(many.update.hosts[0] == "A.TXT");
    REQUIRE(many.update.hosts[1] == "B.TXT");

    const auto missing = parse({"-u"});
    REQUIRE_FALSE(missing.ok);
    REQUIRE(missing.error.find("missing FILE") != std::string::npos);

    const std::string u = dumpfloppy::usage_text();
    REQUIRE(u.find("-uFILE") != std::string::npos);
}

TEST_CASE("expand_inputs error names .img/.ima/.mfm/.86f/.d64/.d71/.d81/.adf",
          "[cli]")
{
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests" /
                     "expand-empty";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    {
        std::ofstream out(dir / "readme.txt");
        REQUIRE(out);
        out << "nope\n";
    }
    std::string err;
    const auto got = dumpfloppy::expand_inputs({dir}, err);
    REQUIRE(got.empty());
    REQUIRE(err.find(".img") != std::string::npos);
    REQUIRE(err.find(".ima") != std::string::npos);
    REQUIRE(err.find(".mfm") != std::string::npos);
    REQUIRE(err.find(".86f") != std::string::npos);
    REQUIRE(err.find(".d64") != std::string::npos);
    REQUIRE(err.find(".d71") != std::string::npos);
    REQUIRE(err.find(".d81") != std::string::npos);
    REQUIRE(err.find(".adf") != std::string::npos);
}

TEST_CASE("expand_inputs directory batch includes .mfm .86f .d64 .d71 .d81 .adf",
          "[cli]")
{
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests" /
                     "expand-exts";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    for (const char* name :
         {"a.ima", "b.mfm", "c.86f", "d.txt", "e.d64", "f.d71", "g.d81", "h.adf"})
    {
        std::ofstream out(dir / name);
        REQUIRE(out);
        out << "x\n";
    }
    std::string err;
    const auto got = dumpfloppy::expand_inputs({dir}, err);
    REQUIRE(err.empty());
    REQUIRE(got.size() == 7);
    REQUIRE(got[0].filename() == "a.ima");
    REQUIRE(got[1].filename() == "b.mfm");
    REQUIRE(got[2].filename() == "c.86f");
    REQUIRE(got[3].filename() == "e.d64");
    REQUIRE(got[4].filename() == "f.d71");
    REQUIRE(got[5].filename() == "g.d81");
    REQUIRE(got[6].filename() == "h.adf");
}
