/**
 * @file test_dos33.cpp
 * @brief DOS 3.3 catalog: type letters, real lengths, deleted files, holes,
 *        hostile chains. Images come from tests/dos33_builder.hpp only.
 */
#include "dos33_builder.hpp"

#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/apple.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/report.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace
{

using namespace dos33_test;

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

std::string report_of(const std::vector<uint8_t>& bytes, bool deleted = true)
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(wrap(bytes, "t.dsk"));
    dumpfloppy::report_options opt{};
    opt.color = false;
    opt.show_deleted = deleted;
    std::ostringstream os;
    dumpfloppy::write_report(a, os, opt);
    return os.str();
}

std::string line_with(const std::string& report, const std::string& needle)
{
    std::istringstream in(report);
    std::string l;
    while (std::getline(in, l))
    {
        if (l.find(needle) != std::string::npos)
        {
            return l;
        }
    }
    return {};
}

} /* namespace */

TEST_CASE("DOS 3.3 type letters follow CATALOG, * marks locked", "[apple][dos33]")
{
    builder b;
    const std::vector<uint8_t> one(10, 0x41);
    b.add("TEXT", t_text, one);
    b.add("INT", t_integer, with_header({4}, 4, 1));
    b.add("SOFT", t_applesoft, with_header({4}, 4, 1));
    b.add("BIN", t_binary, with_header({0x0800, 4}, 4, 1));
    b.add("STYPE", t_s, one);
    b.add("REL", t_relocatable, one);
    b.add("NEWA", t_new_a, one);
    b.add("NEWB", t_new_b, one);
    b.add("LOCKT", t_text | t_locked, one);
    b.add("LOCKB", t_binary | t_locked, with_header({0x2000, 1}, 1, 0));
    b.add("LOCKB2", t_new_b | t_locked, one);
    b.add("ODD", 0x03, one);
    const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(b.bytes());
    REQUIRE(d.present);
    REQUIRE(d.fs == dumpfloppy::apple_fs::dos33);
    REQUIRE(d.entries.size() == 12u);
    const std::vector<std::pair<std::string, std::string>> want = {
        {"TEXT", "T"},    {"INT", "I"},    {"SOFT", "A"},      {"BIN", "B"},
        {"STYPE", "S"},   {"REL", "R"},    {"NEWA", "A2"},     {"NEWB", "B2"},
        {"LOCKT", "*T"},  {"LOCKB", "*B"}, {"LOCKB2", "*B2"},  {"ODD", "$03"}};
    for (const auto& [name, type] : want)
    {
        const auto* e = find(d, name);
        REQUIRE(e != nullptr);
        CHECK(e->type_name == type);
        CHECK(e->locked == (type.front() == '*'));
    }
    const std::string s = report_of(b.bytes());
    CHECK(line_with(s, "LOCKB2").find("*B2") != std::string::npos);
}

TEST_CASE("DOS 3.3 A/I/B lengths come from the file header", "[apple][dos33]")
{
    builder b;
    b.add("HELLO", t_applesoft, with_header({300}, 300, 7));  /* 2 sectors */
    b.add("INTPROG", t_integer, with_header({17}, 17, 3));
    b.add("PIC", t_binary, with_header({0x2000, 8192}, 8192, 0)); /* 33 sectors */
    b.add("EMPTYBIN", t_binary, with_header({0x0300, 0}, 0, 0));
    const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(b.bytes());

    const auto* hello = find(d, "HELLO");
    REQUIRE(hello != nullptr);
    CHECK(hello->dos_has_header);
    CHECK(hello->dos_header_len == 300u);
    CHECK(hello->byte_size == 302u);
    CHECK(hello->dos_sectors == 3u); /* 1 T/S list + 2 data */
    CHECK(hello->dos_len_source == 1u);
    auto payload = dumpfloppy::read_apple_file(d, *hello);
    REQUIRE(payload.size() == 302u);
    CHECK(payload[2] == 7u);
    CHECK(payload[301] == static_cast<uint8_t>(7u + 299u));

    const auto* ip = find(d, "INTPROG");
    REQUIRE(ip != nullptr);
    CHECK(ip->byte_size == 19u);
    CHECK(dumpfloppy::read_apple_file(d, *ip).size() == 19u);

    const auto* pic = find(d, "PIC");
    REQUIRE(pic != nullptr);
    CHECK(pic->dos_load_addr == 0x2000u);
    CHECK(pic->dos_header_len == 8192u);
    CHECK(pic->byte_size == 8196u);
    CHECK(pic->dos_sectors == 34u);
    CHECK(dumpfloppy::read_apple_file(d, *pic).size() == 8196u);

    const auto* eb = find(d, "EMPTYBIN");
    REQUIRE(eb != nullptr);
    CHECK(eb->byte_size == 4u);
    CHECK(eb->dos_load_addr == 0x0300u);

    const std::string s = report_of(b.bytes());
    const std::string pl = line_with(s, "PIC");
    CHECK(pl.find("8196") != std::string::npos);
    CHECK(pl.find("$2000") != std::string::npos);
    CHECK(pl.find("8192") != std::string::npos);
    const std::string hdr = line_with(s, "XXH64 Checksum");
    CHECK(hdr.find("Secs") != std::string::npos);
    CHECK(hdr.find("T/S") != std::string::npos);
    CHECK(hdr.find("Addr") != std::string::npos);
    CHECK(hdr.find("Len") != std::string::npos);
}

TEST_CASE("DOS 3.3 header longer than the data falls back to the T/S list", "[apple][dos33]")
{
    builder b;
    b.add("BAD", t_binary, with_header({0x0800, 5000}, 20, 0)); /* 1 sector */
    const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(b.bytes());
    const auto* e = find(d, "BAD");
    REQUIRE(e != nullptr);
    CHECK_FALSE(e->dos_has_header);
    CHECK(e->dos_len_source == 2u);
    CHECK(e->byte_size == 256u);
    dumpfloppy::apple_read_issues issues{};
    CHECK(dumpfloppy::read_apple_file(d, *e, &issues).size() == 256u);
    CHECK(issues.errors.empty());
    REQUIRE(issues.notes.size() == 1u);
    CHECK(issues.notes[0].find("length word") != std::string::npos);
}

TEST_CASE("DOS 3.3 T ends at $00; B2 uses the T/S list", "[apple][dos33]")
{
    builder b;
    b.add("LOG", t_text, std::vector<uint8_t>(600, 'x')); /* 3 sectors */
    b.add("NB", t_new_b, std::vector<uint8_t>(256, 1));
    const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(b.bytes());
    /* Sequential T: ends at the first $00 (the rest of sector 3 is zero). */
    CHECK(find(d, "LOG")->byte_size == 600u);
    CHECK(find(d, "LOG")->dos_len_source == 3u);
    CHECK(dumpfloppy::read_apple_file(d, *find(d, "LOG")).size() == 600u);
    CHECK(find(d, "NB")->byte_size == 256u);
}

TEST_CASE("DOS 3.3 random-access holes keep later sectors at their offset", "[apple][dos33]")
{
    builder b;
    std::vector<std::vector<uint8_t>> secs(3);
    secs[0].assign(256, 'A');
    secs[2].assign(256, 'C');
    b.add_sparse("RANDOM", t_text, secs);
    const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(b.bytes());
    const auto* e = find(d, "RANDOM");
    REQUIRE(e != nullptr);
    CHECK(e->byte_size == 768u); /* random-access: T/S-list extent, not first $00 */
    CHECK(e->dos_len_source == 2u);
    const auto p = dumpfloppy::read_apple_file(d, *e);
    REQUIRE(p.size() == 768u);
    CHECK(p[0] == 'A');
    CHECK(p[256] == 0u);
    CHECK(p[512] == 'C');
}

TEST_CASE("DOS 3.3 file with two T/S lists", "[apple][dos33]")
{
    builder b;
    std::vector<uint8_t> body(130u * 256u);
    for (std::size_t i = 0; i < body.size(); ++i)
    {
        body[i] = static_cast<uint8_t>(1u + (i / 256u) % 255u);
    }
    b.add("BIG", t_text, body);
    const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(b.bytes());
    const auto* e = find(d, "BIG");
    REQUIRE(e != nullptr);
    CHECK(e->dos_sectors == 132u);
    const auto p = dumpfloppy::read_apple_file(d, *e);
    REQUIRE(p.size() == body.size());
    CHECK(p == body);
}

TEST_CASE("DOS 3.3 deleted files are listed, sized and extracted", "[apple][dos33][deleted]")
{
    builder b;
    b.add("KEEP", t_text, std::vector<uint8_t>(10, 'k'));
    const slot gone = b.add("GONE PROGRAM", t_applesoft | t_locked, with_header({40}, 40, 9));
    const slot gone_b = b.add("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123", t_binary,
                              with_header({0x6000, 100}, 100, 1));
    b.remove(gone);
    b.remove(gone_b);
    const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(b.bytes());
    REQUIRE(d.entries.size() == 3u);

    const auto* g = find(d, "GONE PROGRAM");
    REQUIRE(g != nullptr);
    CHECK(g->deleted);
    CHECK(g->type_name == "*A");
    CHECK(g->ts_track == gone.ts_track);
    CHECK(g->dos_old_track == gone.ts_track);
    CHECK(find(d, "KEEP")->dos_old_track == 0u);
    CHECK(g->ts_sector == gone.ts_sector);
    CHECK(g->byte_size == 42u);
    const auto gp = dumpfloppy::read_apple_file(d, *g);
    REQUIRE(gp.size() == 42u);
    CHECK(gp[2] == 9u);

    /* The 30th name character held the track; 29 survive. */
    const auto* gb = find(d, "ABCDEFGHIJKLMNOPQRSTUVWXYZ012");
    REQUIRE(gb != nullptr);
    CHECK(gb->deleted);
    CHECK(gb->dos_load_addr == 0x6000u);
    CHECK(gb->byte_size == 104u);

    const std::string s = report_of(b.bytes());
    const std::string gl = line_with(s, "GONE PROGRAM");
    REQUIRE_FALSE(gl.empty());
    CHECK(gl.find(" D ") != std::string::npos);
    CHECK(gl.find("42") != std::string::npos);
    CHECK(gl.find("old " + std::to_string(gone.ts_track) + "/") != std::string::npos);
    CHECK(s.find("3 entries shown, 2 deleted on disk") != std::string::npos);
    CHECK(report_of(b.bytes(), false).find("GONE PROGRAM") == std::string::npos);

    const dumpfloppy::analysis a = dumpfloppy::analyse(wrap(b.bytes(), "t.dsk"));
    const auto dest =
        std::filesystem::temp_directory_path() / "dumpfloppy-tests" / "dos33-deleted";
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);
    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    std::ostringstream err;
    REQUIRE(dumpfloppy::extract_files(a, opt, err) == 3);
    std::ifstream in(dest / "?GONE PROGRAM", std::ios::binary);
    REQUIRE(in.good());
    const std::string body((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK(body.size() == 42u);
    CHECK(std::filesystem::exists(dest / "KEEP"));
    std::filesystem::remove_all(dest);
}

TEST_CASE("DOS 3.3 deleted file with reused sectors is a note, not an error",
          "[apple][dos33][deleted]")
{
    builder b;
    const slot gone = b.add("OLD", t_text, std::vector<uint8_t>(10, 'o'));
    b.remove(gone);
    /* Overwrite the old T/S list with an out-of-range data pointer. */
    auto& img = b.bytes();
    const std::size_t l = builder::at(gone.ts_track, gone.ts_sector);
    img[l + 0x0Cu] = 0x30;
    img[l + 0x0Du] = 0x40;
    const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(img);
    const auto* e = find(d, "OLD");
    REQUIRE(e != nullptr);
    dumpfloppy::apple_read_issues issues{};
    CHECK(dumpfloppy::read_apple_file(d, *e, &issues).empty());
    CHECK(issues.errors.empty());
    CHECK_FALSE(issues.notes.empty());
}

TEST_CASE("DOS 3.3 looping T/S list and catalog terminate", "[apple][dos33]")
{
    builder b;
    const slot s = b.add("LOOP", t_text, std::vector<uint8_t>(10, 'l'));
    auto& img = b.bytes();
    const std::size_t l = builder::at(s.ts_track, s.ts_sector);
    img[l + 1u] = s.ts_track; /* list points at itself */
    img[l + 2u] = s.ts_sector;
    const std::size_t cat2 = builder::at(17, 14);
    img[cat2 + 1u] = 17; /* catalog S14 → S15 → S14 … */
    img[cat2 + 2u] = 15;
    const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(img);
    REQUIRE(d.entries.size() == 1u);
    REQUIRE_FALSE(d.problems.empty());
    CHECK(d.problems[0].find("loops") != std::string::npos);
    dumpfloppy::apple_read_issues issues{};
    CHECK(dumpfloppy::read_apple_file(d, d.entries[0], &issues).size() == 10u);
    REQUIRE_FALSE(issues.errors.empty());
    CHECK(issues.errors[0].find("loops") != std::string::npos);
}
