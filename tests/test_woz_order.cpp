/**
 * @file test_woz_order.cpp
 * @brief WOZ physical sector order -> DOS 3.3 / ProDOS, and 13-sector refusal.
 *
 * Expected layouts come from the skew formulas in tests/woz_builder.hpp
 * (DOS 13L mod 15, ProDOS 2p mod 15) and tests/prodos_builder.hpp, never
 * from the tables in src/apple.cpp.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/apple.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/foreign.hpp"

#include "prodos_builder.hpp"
#include "woz_builder.hpp"

#include <catch2/catch_test_macros.hpp>

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

/** @brief DOS-order image -> physical order via the builder's formula. */
std::vector<uint8_t> dos_to_physical(const std::vector<uint8_t>& dos)
{
    std::vector<uint8_t> phys(dos.size(), 0);
    for (std::size_t base = 0; base + 4096u <= dos.size(); base += 4096u)
    {
        for (uint8_t l = 0; l < 16u; ++l)
        {
            std::memcpy(phys.data() + base + dumpfloppy_test::dos_logical_to_physical(l) * 256u,
                        dos.data() + base + l * 256u, 256u);
        }
    }
    return phys;
}

/** @brief DOS 3.3 140K: HELLO = 15 data sectors at T18 S1..S15, distinct bytes. */
std::vector<uint8_t> make_dos33_multi(std::vector<uint8_t>& payload)
{
    std::vector<uint8_t> img(dumpfloppy::k_apple_dos33_140k, 0);
    auto sec = [&](unsigned t, unsigned s) { return img.data() + (t * 16u + s) * 256u; };
    uint8_t* vtoc = sec(17, 0);
    vtoc[1] = 17;
    vtoc[2] = 15;
    vtoc[3] = 3;
    vtoc[6] = 254;
    vtoc[0x27] = 122;
    vtoc[0x34] = 35;
    vtoc[0x35] = 16;
    vtoc[0x37] = 1;
    for (unsigned s = 15; s >= 2; --s)
    {
        sec(17, s)[1] = 17;
        sec(17, s)[2] = static_cast<uint8_t>(s - 1);
    }
    sec(17, 1)[1] = 0;
    uint8_t* e = sec(17, 15) + 0x0B;
    e[0] = 18;
    e[1] = 0;
    e[2] = 0x04;
    const char* name = "HELLO";
    for (int i = 0; i < 30; ++i)
    {
        e[3 + i] = static_cast<uint8_t>((i < 5 ? name[i] : ' ') | 0x80);
    }
    e[33] = 16;
    uint8_t* ts = sec(18, 0);
    payload = pattern(15u * 256u, 41);
    for (unsigned s = 1; s < 16u; ++s)
    {
        ts[0x0C + (s - 1) * 2] = 18;
        ts[0x0D + (s - 1) * 2] = static_cast<uint8_t>(s);
        std::memcpy(sec(18, s), payload.data() + (s - 1) * 256u, 256u);
    }
    return img;
}

dumpfloppy::floppy_image wrap(std::vector<uint8_t> bytes, const char* path)
{
    dumpfloppy::floppy_image img{};
    img.bytes = std::move(bytes);
    img.path = path;
    return img;
}

} /* namespace */

TEST_CASE("sector-order tables agree by composition", "[woz][order]")
{
    /* ProDOS sector p sits at physical 2p mod 15 (15 fixed); DOS logical L
     * at 13L mod 15. DOS-order -> ProDOS must equal physical -> ProDOS
     * composed with DOS -> physical. */
    for (unsigned l = 0; l < 16u; ++l)
    {
        unsigned back = 0;
        while (dumpfloppy_test::dos_logical_to_physical(static_cast<uint8_t>(back)) !=
               dumpfloppy_test::dos_logical_to_physical(static_cast<uint8_t>(l)))
        {
            ++back;
        }
        REQUIRE(back == l); /* the skew is a permutation */
    }
    const auto dos = pattern(dumpfloppy::k_apple_dos33_140k, 51);
    const auto phys = dos_to_physical(dos);
    REQUIRE(dumpfloppy::apple_physical_order_to_dos(phys) == dos);
    REQUIRE(dumpfloppy::apple_physical_order_to_prodos(phys) ==
            dumpfloppy::apple_dos_order_to_prodos(dos));
    const auto po = pattern(dumpfloppy::k_apple_dos33_140k, 52);
    REQUIRE(dumpfloppy::apple_dos_order_to_prodos(prodos_test::to_dos_order(po)) == po);
    REQUIRE(dumpfloppy::apple_physical_order_to_prodos(
                dos_to_physical(prodos_test::to_dos_order(po))) == po);
}

TEST_CASE("WOZ DOS 3.3 reads a file spread over every sector", "[woz][dos33]")
{
    std::vector<uint8_t> payload;
    const auto dos = make_dos33_multi(payload);
    const auto woz = dumpfloppy_test::dos_to_woz2(dos);
    REQUIRE(dumpfloppy::assemble_woz(woz) == dos_to_physical(dos));
    const auto a = dumpfloppy::analyse(wrap(woz, "t.woz"));
    REQUIRE(a.unsupported.empty());
    REQUIRE(a.apple.present);
    REQUIRE(a.apple.fs == dumpfloppy::apple_fs::dos33);
    REQUIRE(a.apple.entries.size() == 1u);
    const auto got = dumpfloppy::read_apple_file(a.apple, a.apple.entries[0]);
    REQUIRE(got == payload);
}

TEST_CASE("WOZ ProDOS reads sapling and subdirectory files", "[woz][prodos]")
{
    prodos_test::volume v(280, "WOZVOL");
    v.add_file(v.root(), "BIG", 0x06, 0x2000, pattern(30000, 61));
    const int sub = v.add_subdir(v.root(), "D");
    v.add_file(sub, "SMALL", 0x04, 0, pattern(700, 62));
    const auto po = v.image();
    const auto woz = dumpfloppy_test::dos_to_woz2(prodos_test::to_dos_order(po));
    const auto a = dumpfloppy::analyse(wrap(woz, "t.woz"));
    REQUIRE(a.apple.present);
    REQUIRE(a.apple.fs == dumpfloppy::apple_fs::prodos);
    REQUIRE(a.apple.volume == po);
    bool big = false;
    bool small = false;
    for (const auto& e : a.apple.entries)
    {
        if (e.name == "BIG")
        {
            big = dumpfloppy::read_apple_file(a.apple, e) == pattern(30000, 61);
        }
        if (e.name == "D/SMALL")
        {
            small = dumpfloppy::read_apple_file(a.apple, e) == pattern(700, 62);
        }
    }
    REQUIRE(big);
    REQUIRE(small);
}

TEST_CASE("13-sector DOS 3.2 WOZ is reported unsupported", "[woz][dos32]")
{
    const std::vector<uint8_t> blank(dumpfloppy::k_apple_dos33_140k, 0);
    const auto woz = dumpfloppy_test::dos_to_woz2(blank, true);
    dumpfloppy::woz_scan scan{};
    (void)dumpfloppy::assemble_woz(woz, &scan);
    REQUIRE(scan.addr13 == 35u * 13u);
    REQUIRE(scan.addr16 == 0u);
    const auto a = dumpfloppy::analyse(wrap(woz, "t.woz"));
    REQUIRE_FALSE(a.unsupported.empty());
    REQUIRE(a.unsupported.find("13-sector") != std::string::npos);
    REQUIRE_FALSE(a.apple.present);
    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "woz13";
    std::filesystem::create_directories(opt.dest_dir);
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) < 0);
    REQUIRE(err.str().find("13-sector") != std::string::npos);

    /* A 16-sector disk is not flagged. */
    std::vector<uint8_t> payload;
    const auto ok = dumpfloppy::analyse(
        wrap(dumpfloppy_test::dos_to_woz2(make_dos33_multi(payload)), "ok.woz"));
    REQUIRE(ok.unsupported.empty());
}
