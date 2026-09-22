/**
 * @file test_cli.cpp
 * @brief Help/version/no-args/order-independence contracts.
 */
#include "dumpfloppy/cli.hpp"
#include "dumpfloppy/version.hpp"

#include <catch2/catch_test_macros.hpp>
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

    /* .ima suffix is an image operand, not a glob. */
    const auto img = parse({"-x", "disk.ima"});
    REQUIRE(img.ok);
    REQUIRE(img.extract.enabled);
    REQUIRE(img.extract.patterns.empty());
    REQUIRE(img.inputs.size() == 1);
    REQUIRE(img.inputs[0] == "disk.ima");

    const auto named = parse({"disk.ima", "-x", "591.PKD"});
    REQUIRE(named.extract.patterns[0] == "591.PKD");
}
