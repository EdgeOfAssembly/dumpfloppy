/**
 * @file test_prodos.cpp
 * @brief ProDOS catalog: subdirectories, seedling/sapling/tree, sparse,
 *        extended forks, aux type, case bits, DOS order, 2IMG header, -x.
 *
 * Every image is synthesised by tests/prodos_builder.hpp.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/apple.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/report.hpp"

#include "prodos_builder.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace
{

using prodos_test::pattern;

struct fixture
{
    std::vector<uint8_t> image;
    std::vector<uint8_t> seed;   /* 300 B seedling */
    std::vector<uint8_t> sap;    /* 70000 B sapling */
    std::vector<uint8_t> sparse; /* sapling with zero holes */
    std::vector<uint8_t> tree;   /* 200000 B tree */
    std::vector<uint8_t> deep;   /* file in SUB/INNER */
    std::vector<uint8_t> dfork;
    std::vector<uint8_t> rfork;
};

/** @brief 1600-block (800K) volume exercising every storage type. */
fixture make_fixture()
{
    fixture f;
    f.seed = pattern(300, 1);
    f.sap = pattern(70000, 2);
    f.sparse = pattern(512u * 40u + 17u, 3);
    for (std::size_t i = 512u * 5u; i < 512u * 30u; ++i)
    {
        f.sparse[i] = 0; /* blocks 5-29 become sparse */
    }
    f.tree = pattern(200000, 4);
    f.deep = pattern(1234, 5);
    f.dfork = pattern(5000, 6);
    f.rfork = pattern(777, 7);

    /* GS/OS case bits: bit 15 on, bits 13..8 = chars 1..6 -> "Testvol". */
    prodos_test::volume v(1600, "TESTVOL", static_cast<uint16_t>(0x8000u | 0x3F00u));
    const int root = v.root();
    v.add_file(root, "SEED", 0x06, 0x2000, f.seed);
    v.add_file(root, "SAPLING", 0x04, 0x0050, f.sap, false, 0, 0x01); /* locked */
    v.add_file(root, "SPARSE", 0x06, 0x0000, f.sparse, true);
    v.add_file(root, "TREE.FILE", 0xB3, 0xDB07, f.tree);
    v.add_file(root, "README", 0x04, 0x0000, pattern(10, 8), false,
               static_cast<uint16_t>(0x8000u | 0x3E00u)); /* "Readme" */
    v.add_file(root, "GONE", 0x06, 0x0000, pattern(10, 9));
    v.delete_last(root);
    const int sub = v.add_subdir(root, "SUB");
    v.add_file(sub, "A", 0x00, 0x0000, std::vector<uint8_t>{});
    const int inner = v.add_subdir(sub, "INNER");
    v.add_file(inner, "DEEP.BIN", 0x06, 0x0803, f.deep);
    v.add_extended(root, "FORKED", 0xB3, 0x0000, f.dfork, f.rfork);
    /* Push the volume directory past its first block (13 slots). */
    for (int i = 0; i < 10; ++i)
    {
        v.add_file(root, "F" + std::to_string(i), 0x06, static_cast<uint16_t>(i),
                   pattern(static_cast<std::size_t>(i) * 100u + 1u, 100u + static_cast<uint32_t>(i)));
    }
    f.image = v.image();
    return f;
}

const dumpfloppy::apple_file* find(const dumpfloppy::apple_disk& d, const std::string& name)
{
    for (const auto& e : d.entries)
    {
        if (e.name == name)
        {
            return &e;
        }
    }
    return nullptr;
}

dumpfloppy::floppy_image wrap(std::vector<uint8_t> bytes, const char* path)
{
    dumpfloppy::floppy_image img{};
    img.bytes = std::move(bytes);
    img.path = path;
    return img;
}

std::vector<uint8_t> slurp(const std::filesystem::path& p)
{
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void check_catalog(const dumpfloppy::apple_disk& d, const fixture& f)
{
    REQUIRE(d.present);
    REQUIRE(d.fs == dumpfloppy::apple_fs::prodos);
    REQUIRE(d.volume_name == "Testvol");
    REQUIRE(d.total_blocks == 1600u);
    auto payload = [&](const std::string& n)
    {
        const auto* e = find(d, n);
        REQUIRE(e != nullptr);
        return dumpfloppy::read_apple_file(d, *e);
    };
    REQUIRE(find(d, "SEED")->storage == 1u);
    REQUIRE(find(d, "SEED")->aux_type == 0x2000u);
    REQUIRE(payload("SEED") == f.seed);
    REQUIRE(find(d, "SAPLING")->storage == 2u);
    REQUIRE(find(d, "SAPLING")->locked);
    REQUIRE_FALSE(find(d, "SEED")->locked);
    REQUIRE(payload("SAPLING") == f.sap);
    REQUIRE(payload("SPARSE") == f.sparse);
    REQUIRE(find(d, "TREE.FILE")->storage == 3u);
    REQUIRE(find(d, "TREE.FILE")->type_name == "S16");
    REQUIRE(find(d, "TREE.FILE")->aux_type == 0xDB07u);
    REQUIRE(payload("TREE.FILE") == f.tree);
    REQUIRE(find(d, "Readme") != nullptr);
    REQUIRE(find(d, "?ONE") != nullptr);
    REQUIRE(find(d, "?ONE")->deleted);
    REQUIRE(find(d, "SUB")->type_name == "DIR");
    REQUIRE(find(d, "SUB/A") != nullptr);
    REQUIRE(payload("SUB/A").empty());
    REQUIRE(find(d, "SUB/INNER")->type_name == "DIR");
    REQUIRE(find(d, "SUB/INNER/DEEP.BIN")->aux_type == 0x0803u);
    REQUIRE(payload("SUB/INNER/DEEP.BIN") == f.deep);
    REQUIRE(find(d, "FORKED")->extended);
    REQUIRE_FALSE(find(d, "FORKED")->resource_fork);
    REQUIRE(payload("FORKED") == f.dfork);
    REQUIRE(find(d, "FORKED.rsrc")->resource_fork);
    REQUIRE(payload("FORKED.rsrc") == f.rfork);
    for (int i = 0; i < 10; ++i)
    {
        const std::string n = "F" + std::to_string(i);
        REQUIRE(find(d, n) != nullptr);
        REQUIRE(find(d, n)->aux_type == static_cast<uint16_t>(i));
        REQUIRE(payload(n) == pattern(static_cast<std::size_t>(i) * 100u + 1u,
                                      100u + static_cast<uint32_t>(i)));
    }
}

} /* namespace */

TEST_CASE("ProDOS reads every storage type, subdirs, forks", "[prodos]")
{
    const fixture f = make_fixture();
    check_catalog(dumpfloppy::parse_apple(f.image), f);
}

TEST_CASE("ProDOS in DOS order is remapped (.do, 2IMG format 0)", "[prodos][order]")
{
    const fixture f = make_fixture();
    const auto dos = prodos_test::to_dos_order(f.image);
    REQUIRE_FALSE(dumpfloppy::parse_apple(dos).present);
    const auto d = dumpfloppy::parse_apple_ordered(dos, dumpfloppy::apple_order::dos);
    check_catalog(d, f);
    REQUIRE(d.dos_order);
    const auto u = dumpfloppy::parse_apple_ordered(dos, dumpfloppy::apple_order::unknown);
    REQUIRE(u.present);
    REQUIRE(u.dos_order);
    REQUIRE_FALSE(
        dumpfloppy::parse_apple_ordered(dos, dumpfloppy::apple_order::prodos).present);

    const auto a = dumpfloppy::analyse(wrap(dos, "t.do"));
    check_catalog(a.apple, f);
    const auto mg = dumpfloppy::analyse(wrap(prodos_test::wrap_2img(dos, 0), "t.2mg"));
    check_catalog(mg.apple, f);
    REQUIRE(mg.foreign.img2mg_format == 0u);
    /* .po stays strict ProDOS order. */
    REQUIRE_FALSE(dumpfloppy::analyse(wrap(dos, "t.po")).apple.present);
}

TEST_CASE("2IMG header: offset/length at 0x18/0x1C", "[prodos][2img]")
{
    const fixture f = make_fixture();
    const auto mg = prodos_test::wrap_2img(f.image, 1);
    const auto a = dumpfloppy::analyse(wrap(mg, "t.2mg"));
    REQUIRE(a.foreign.data_offset == 64u);
    REQUIRE(a.foreign.data_length == f.image.size());
    REQUIRE(a.foreign.img2mg_format == 1u);
    check_catalog(a.apple, f);
    dumpfloppy::report_options opt{};
    opt.color = false;
    std::ostringstream os;
    dumpfloppy::write_report(a, os, opt);
    const std::string s = os.str();
    REQUIRE(s.find("Payload           819200 bytes @ 64") != std::string::npos);
    REQUIRE(s.find("SUB/INNER/DEEP.BIN") != std::string::npos);
    REQUIRE(s.find("$DB07") != std::string::npos);
    REQUIRE(s.find("not decoded") == std::string::npos);
}

TEST_CASE("-x extracts a 2IMG ProDOS volume incl. subdirs and forks", "[prodos][extract]")
{
    const fixture f = make_fixture();
    const auto a = dumpfloppy::analyse(wrap(prodos_test::wrap_2img(f.image, 1), "t.2mg"));
    const auto dest = std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "prodos-x";
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);
    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    const int n = dumpfloppy::extract_files(a, opt, err);
    INFO(err.str());
    REQUIRE(n > 0);
    REQUIRE(slurp(dest / "TREE.FILE") == f.tree);
    REQUIRE(slurp(dest / "SPARSE") == f.sparse);
    REQUIRE(slurp(dest / "SUB_INNER_DEEP.BIN") == f.deep);
    REQUIRE(slurp(dest / "FORKED") == f.dfork);
    REQUIRE(slurp(dest / "FORKED.rsrc") == f.rfork);
    REQUIRE_FALSE(std::filesystem::exists(dest / "SUB"));
}

TEST_CASE("-x on a 2IMG block image without a catalog names the real reason",
          "[prodos][2img][extract]")
{
    std::vector<uint8_t> blank(280u * 512u, 0);
    const auto a = dumpfloppy::analyse(wrap(prodos_test::wrap_2img(blank, 1), "t.2mg"));
    REQUIRE_FALSE(a.apple.present);
    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "prodos-none";
    std::filesystem::create_directories(opt.dest_dir);
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) < 0);
    REQUIRE(err.str().find("flux/nibble") == std::string::npos);
    REQUIRE(err.str().find("ProDOS-order block image") != std::string::npos);
}

TEST_CASE("ProDOS directory loops and bad pointers stay bounded", "[prodos][robust]")
{
    prodos_test::volume v(280, "LOOP");
    const int root = v.root();
    const int sub = v.add_subdir(root, "SUB");
    v.add_file(sub, "X", 0x06, 0, pattern(600, 1));
    /* Subdirectory chain points back at itself and its entry at the root. */
    uint8_t* sb = v.blk(v.dir_key(sub));
    sb[2] = static_cast<uint8_t>(v.dir_key(sub));
    sb[3] = 0;
    auto img = v.image();
    /* Root's last block links back to block 2. */
    img[5u * 512u + 2u] = 2;
    const auto d = dumpfloppy::parse_apple(img);
    REQUIRE(d.present);
    REQUIRE(d.entries.size() == 2u);

    /* A file whose key/index point past the volume reads as zeros, not OOB. */
    prodos_test::volume w(280, "BAD");
    w.add_file(w.root(), "BIG", 0x06, 0, pattern(3000, 2));
    auto bad = w.image();
    const std::size_t e = 2u * 512u + 4u + 0x27u;
    bad[e + 0x11u] = 0xFF;
    bad[e + 0x12u] = 0xFF;
    const auto bd = dumpfloppy::parse_apple(bad);
    REQUIRE(bd.entries.size() == 1u);
    const auto p = dumpfloppy::read_apple_file(bd, bd.entries[0]);
    REQUIRE(p.size() == 3000u);
}
