/**
 * @file test_prodos.cpp
 * @brief ProDOS catalog: subdirectories, seedling/sapling/tree, sparse,
 *        extended forks, aux type, case bits, DOS order, 2IMG header, -x.
 *
 * Every image is synthesised by tests/prodos_builder.hpp.
 */
#include "test_tmp.hpp"
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/apple.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/report.hpp"

#include "prodos_builder.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
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
    const auto dest = dumpfloppy_test::tmp_root() / "dumpfloppy-tests" / "prodos-x";
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
    opt.dest_dir = dumpfloppy_test::tmp_root() / "dumpfloppy-tests" / "prodos-none";
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

    /* A key block past the volume is reported, never read out of bounds. */
    prodos_test::volume w(280, "BAD");
    w.add_file(w.root(), "BIG", 0x06, 0, pattern(3000, 2));
    auto bad = w.image();
    const std::size_t e = 2u * 512u + 4u + 0x27u;
    bad[e + 0x11u] = 0xFF;
    bad[e + 0x12u] = 0xFF;
    const auto bd = dumpfloppy::parse_apple(bad);
    REQUIRE(bd.entries.size() == 1u);
    dumpfloppy::apple_read_issues iss{};
    const auto p = dumpfloppy::read_apple_file(bd, bd.entries[0], &iss);
    REQUIRE(p.empty());
    REQUIRE_FALSE(iss.errors.empty());
}

namespace
{

/** @brief Small 140K volume used by the review repros. */
std::vector<uint8_t> make_140k()
{
    prodos_test::volume v(280, "SMALL");
    v.add_file(v.root(), "HELLO", 0x04, 0, pattern(700, 21));
    const int sub = v.add_subdir(v.root(), "DOCS");
    v.add_file(sub, "NOTE", 0x04, 0, pattern(9000, 22));
    return v.image();
}

int extract_to(const dumpfloppy::analysis& a, const char* leaf, std::string& err_out)
{
    const auto dest = dumpfloppy_test::tmp_root() / "dumpfloppy-tests" / leaf;
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);
    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    const int rc = dumpfloppy::extract_files(a, opt, err);
    err_out = err.str();
    return rc;
}

std::uintmax_t dir_bytes(const char* leaf)
{
    std::uintmax_t n = 0;
    for (const auto& e : std::filesystem::recursive_directory_iterator(
             dumpfloppy_test::tmp_root() / "dumpfloppy-tests" / leaf))
    {
        if (e.is_regular_file())
        {
            n += e.file_size();
        }
    }
    return n;
}

} /* namespace */

TEST_CASE("review repro dos_order_prodos: DOS-order 140K ProDOS decodes", "[prodos][order][review]")
{
    const auto po = make_140k();
    const auto dos = prodos_test::to_dos_order(po);
    /* Known layout, stated without any table: ProDOS block 0 is DOS
     * sectors 0 and 14 of track 0; block 7 is DOS sectors 1 and 15. */
    REQUIRE(std::equal(po.begin() + 256, po.begin() + 512, dos.begin() + 14 * 256));
    REQUIRE(std::equal(po.begin() + 7 * 512, po.begin() + 7 * 512 + 256, dos.begin() + 256));
    REQUIRE(std::equal(po.begin() + 7 * 512 + 256, po.begin() + 8 * 512, dos.begin() + 15 * 256));
    /* Block 2 (volume dir) is DOS sectors 11 and 10 of track 0. */
    REQUIRE(std::equal(po.begin() + 1024, po.begin() + 1280, dos.begin() + 11 * 256));
    REQUIRE(std::equal(po.begin() + 1280, po.begin() + 1536, dos.begin() + 10 * 256));

    for (const char* name : {"t.do", "t.dsk", "t.po", "t.img"})
    {
        INFO(name);
        const auto a = dumpfloppy::analyse(wrap(dos, name));
        REQUIRE(a.apple.present);
        REQUIRE(a.apple.dos_order);
        const auto* note = find(a.apple, "DOCS/NOTE");
        REQUIRE(note != nullptr);
        REQUIRE(dumpfloppy::read_apple_file(a.apple, *note) == pattern(9000, 22));
    }
    /* A ProDOS-order image named .dsk/.do is not remapped. */
    for (const char* name : {"t.dsk", "t.do"})
    {
        const auto a = dumpfloppy::analyse(wrap(po, name));
        REQUIRE(a.apple.present);
        REQUIRE_FALSE(a.apple.dos_order);
    }
    const auto mg = dumpfloppy::analyse(wrap(prodos_test::wrap_2img(dos, 0), "t.2mg"));
    REQUIRE(mg.apple.present);
    REQUIRE(mg.apple.dos_order);
    std::string err;
    REQUIRE(extract_to(mg, "prodos-do", err) == 2);
    REQUIRE(slurp(dumpfloppy_test::tmp_root() / "dumpfloppy-tests" / "prodos-do" /
                  "DOCS_NOTE") == pattern(9000, 22));
}

TEST_CASE("review repro eof_amplification: huge EOFs cannot amplify -x", "[prodos][extract][review]")
{
    /* Like the reviewer's amp.py: every entry is a seedling, key 7, EOF $FFFFFF. */
    std::vector<uint8_t> d(280u * 512u, 0);
    const uint16_t chain[4] = {2, 3, 4, 5};
    for (int bi = 0; bi < 4; ++bi)
    {
        uint8_t* b = d.data() + chain[bi] * 512u;
        b[0] = static_cast<uint8_t>(bi == 0 ? 0 : chain[bi - 1]);
        b[2] = static_cast<uint8_t>(bi == 3 ? 0 : chain[bi + 1]);
        for (int i = 0; i < 13; ++i)
        {
            uint8_t* e = b + 4 + i * 0x27;
            if (bi == 0 && i == 0)
            {
                e[0] = 0xF5;
                std::memcpy(e + 1, "AMPLI", 5);
                e[0x1F] = 0x27;
                e[0x20] = 13;
                e[0x23] = 6;
                e[0x25] = 0x18;
                e[0x26] = 0x01;
                continue;
            }
            const std::string nm = "H" + std::to_string(bi * 13 + i);
            e[0] = static_cast<uint8_t>(0x10u | nm.size());
            std::memcpy(e + 1, nm.data(), nm.size());
            e[0x10] = 6;
            e[0x11] = 7;
            e[0x15] = 0xFF;
            e[0x16] = 0xFF;
            e[0x17] = 0xFF;
        }
    }
    const auto a = dumpfloppy::analyse(wrap(d, "amp.po"));
    REQUIRE(a.apple.present);
    REQUIRE(a.apple.entries.size() == 51u);
    std::string err;
    REQUIRE(extract_to(a, "prodos-amp", err) < 0);
    REQUIRE(err.find("exceeds the volume size") != std::string::npos);
    REQUIRE(dir_bytes("prodos-amp") <= 4u * d.size());
    const auto p = dumpfloppy::read_apple_file(a.apple, a.apple.entries[0]);
    REQUIRE(p.size() == 512u);
}

TEST_CASE("review repro silent_zero_fill: bad pointers and 2IMG headers warn", "[prodos][2img][review]")
{
    /* Sapling whose index is all $FFFF, a seedling key $FFFF, a tree key $FFF0. */
    prodos_test::volume v(280, "OOR");
    v.add_file(v.root(), "OOR", 0x06, 0, pattern(600, 31));
    v.add_file(v.root(), "OK", 0x06, 0, pattern(100, 32));
    auto img = v.image();
    const auto pre = dumpfloppy::parse_apple(img);
    const uint16_t ix = find(pre, "OOR")->key;
    std::fill(img.begin() + ix * 512, img.begin() + ix * 512 + 512, 0xFF);
    const auto a = dumpfloppy::analyse(wrap(img, "oor.po"));
    dumpfloppy::apple_read_issues iss{};
    (void)dumpfloppy::read_apple_file(a.apple, *find(a.apple, "OOR"), &iss);
    REQUIRE_FALSE(iss.errors.empty());
    std::string err;
    REQUIRE(extract_to(a, "prodos-oor", err) < 0);
    REQUIRE(err.find("out of range") != std::string::npos);
    dumpfloppy::report_options opt{};
    opt.color = false;
    std::ostringstream os;
    dumpfloppy::write_report(a, os, opt);
    REQUIRE(os.str().find("Warning: 1 file(s)") != std::string::npos);

    const auto good = make_140k();
    auto mg = prodos_test::wrap_2img(good, 1);
    {
        const std::vector<uint8_t> trunc(mg.begin(), mg.begin() + 5000);
        const auto t = dumpfloppy::analyse(wrap(trunc, "trunc.2mg"));
        REQUIRE_FALSE(t.foreign.problems.empty());
        REQUIRE(extract_to(t, "prodos-trunc", err) < 0);
        REQUIRE(err.find("warning") != std::string::npos);
    }
    {
        auto h = mg;
        h[8] = 0;
        h[9] = 0; /* header length 0 */
        const auto t = dumpfloppy::analyse(wrap(h, "hdr0.2mg"));
        REQUIRE(t.apple.present);
        REQUIRE(extract_to(t, "prodos-hdr0", err) < 0);
        REQUIRE(err.find("header length 0") != std::string::npos);
    }
    {
        auto h = mg;
        h[0x1C] = h[0x1D] = h[0x1E] = h[0x1F] = 0xFF; /* data length $FFFFFFFF */
        const auto t = dumpfloppy::analyse(wrap(h, "huge.2mg"));
        REQUIRE(t.apple.present);
        REQUIRE(extract_to(t, "prodos-huge", err) < 0);
        REQUIRE(err.find("data length") != std::string::npos);
    }
    {
        /* A clean image stays clean: no warnings, rc = files written. */
        const auto t = dumpfloppy::analyse(wrap(mg, "ok.2mg"));
        REQUIRE(t.foreign.problems.empty());
        REQUIRE(t.apple.problems.empty());
        REQUIRE(extract_to(t, "prodos-ok", err) == 2);
        REQUIRE(err.empty());
    }
}
