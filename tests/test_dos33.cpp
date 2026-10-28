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
#include <nlohmann/json.hpp>

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

namespace
{

std::string json_of(const std::vector<uint8_t>& bytes, bool deleted = true)
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(wrap(bytes, "t.dsk"));
    dumpfloppy::report_options opt{};
    opt.color = false;
    opt.show_deleted = deleted;
    std::ostringstream os;
    dumpfloppy::write_json_begin(os);
    dumpfloppy::write_json_image(a, os, opt, true);
    dumpfloppy::write_json_end(os);
    return os.str();
}

const nlohmann::json* json_entry(const nlohmann::json& doc, const std::string& name)
{
    for (const auto& e : doc.at("images").at(0).at("entries"))
    {
        if (e.at("name").get<std::string>() == name)
        {
            return &e;
        }
    }
    return nullptr;
}

} /* namespace */

TEST_CASE("DOS 3.3 JSON carries the catalog columns", "[apple][dos33][json]")
{
    builder b;
    b.add("TEXT", t_text, std::vector<uint8_t>{'H', 'I', 0, 'x', 'y'});
    const slot bin = b.add("BIN", t_binary, with_header({0x0800, 4}, 4, 1));
    b.add("LOCKA", t_applesoft | t_locked, with_header({6}, 6, 2));
    b.add("NEWB", t_new_b, std::vector<uint8_t>(10, 1));
    const slot gone = b.add("GONE", t_integer | t_locked, with_header({40}, 40, 9));
    b.remove(gone);
    const auto doc = nlohmann::json::parse(json_of(b.bytes()));

    /* Existing keys unchanged. */
    const auto* jb = json_entry(doc, "BIN");
    REQUIRE(jb != nullptr);
    for (const char* k : {"path", "name", "size", "deleted", "directory", "first_cluster", "type",
                          "xxh64"})
    {
        CHECK(jb->contains(k));
    }
    CHECK(jb->at("type").get<std::string>() == "B");
    CHECK(jb->at("size").get<unsigned>() == 8u);
    const auto& db = jb->at("dos33");
    CHECK_FALSE(db.at("deleted").get<bool>());
    CHECK_FALSE(db.at("locked").get<bool>());
    CHECK(db.at("type_letter").get<std::string>() == "B");
    CHECK(db.at("type_byte").get<unsigned>() == 0x04u);
    CHECK(db.at("sectors").get<unsigned>() == 2u);
    CHECK(db.at("ts_track").get<unsigned>() == bin.ts_track);
    CHECK(db.at("ts_sector").get<unsigned>() == bin.ts_sector);
    CHECK(db.at("old_ts_track").is_null());
    CHECK(db.at("old_ts_sector").is_null());
    CHECK(db.at("load_address").get<unsigned>() == 0x0800u);
    CHECK(db.at("load_address_hex").get<std::string>() == "$0800");
    CHECK(db.at("header_length").get<unsigned>() == 4u);
    CHECK(db.at("real_length").get<unsigned>() == 8u);
    CHECK(db.at("length_source").get<std::string>() == "header");

    const auto& dl = json_entry(doc, "LOCKA")->at("dos33");
    CHECK(dl.at("locked").get<bool>());
    CHECK(dl.at("type_letter").get<std::string>() == "A");
    CHECK(json_entry(doc, "LOCKA")->at("type").get<std::string>() == "*A");
    CHECK(dl.at("load_address").is_null());
    CHECK(dl.at("real_length").get<unsigned>() == 8u);

    const auto& dt = json_entry(doc, "TEXT")->at("dos33");
    CHECK(dt.at("real_length").get<unsigned>() == 2u);
    CHECK(dt.at("length_source").get<std::string>() == "text_nul");
    CHECK(dt.at("header_length").is_null());

    const auto& dn = json_entry(doc, "NEWB")->at("dos33");
    CHECK(dn.at("type_letter").get<std::string>() == "B2");
    CHECK(dn.at("length_source").get<std::string>() == "ts_list");
    CHECK(dn.at("load_address").is_null());

    const auto* jg = json_entry(doc, "GONE");
    REQUIRE(jg != nullptr);
    CHECK(jg->at("deleted").get<bool>());
    const auto& dg = jg->at("dos33");
    CHECK(dg.at("deleted").get<bool>());
    CHECK(dg.at("locked").get<bool>());
    CHECK(dg.at("type_letter").get<std::string>() == "I");
    CHECK(dg.at("old_ts_track").get<unsigned>() == gone.ts_track);
    CHECK(dg.at("old_ts_sector").get<unsigned>() == gone.ts_sector);
    CHECK(dg.at("real_length").get<unsigned>() == 42u);

    /* --no-deleted drops the row from JSON too. */
    const auto doc2 = nlohmann::json::parse(json_of(b.bytes(), false));
    CHECK(json_entry(doc2, "GONE") == nullptr);
}

/* ---- --undelete (0.43) ------------------------------------------------- */

namespace
{

std::string report_undel(const std::vector<uint8_t>& bytes, bool undelete)
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(wrap(bytes, "t.dsk"));
    dumpfloppy::report_options opt{};
    opt.color = false;
    opt.undelete = undelete;
    std::ostringstream os;
    dumpfloppy::write_report(a, os, opt);
    return os.str();
}

nlohmann::json json_undel(const std::vector<uint8_t>& bytes, bool undelete)
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(wrap(bytes, "t.dsk"));
    dumpfloppy::report_options opt{};
    opt.color = false;
    opt.undelete = undelete;
    std::ostringstream os;
    dumpfloppy::write_json_begin(os);
    dumpfloppy::write_json_image(a, os, opt, true);
    dumpfloppy::write_json_end(os);
    return nlohmann::json::parse(os.str());
}

/** @brief Fresh extract dir under the temp dir; caller removes it. */
std::filesystem::path undel_dest(const char* leaf)
{
    const auto dest = std::filesystem::temp_directory_path() / "dumpfloppy-tests" / leaf;
    std::filesystem::remove_all(dest);
    std::filesystem::create_directories(dest);
    return dest;
}

int extract_undel(const std::vector<uint8_t>& bytes, const std::filesystem::path& dest,
                  bool undelete, std::string* err_out = nullptr)
{
    const dumpfloppy::analysis a = dumpfloppy::analyse(wrap(bytes, "t.dsk"));
    dumpfloppy::extract_options opt{};
    opt.enabled = true;
    opt.dest_dir = dest;
    opt.undelete = undelete;
    std::ostringstream err;
    const int n = dumpfloppy::extract_files(a, opt, err);
    if (err_out != nullptr)
    {
        *err_out = err.str();
    }
    return n;
}

std::string slurp(const std::filesystem::path& p)
{
    std::ifstream in(p, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

} /* namespace */

TEST_CASE("DOS 3.3 --undelete: full recovery", "[apple][dos33][undelete]")
{
    builder b;
    b.add("KEEP", t_text, std::vector<uint8_t>(10, 'k'));
    const auto body = with_header({0x0800, 400}, 400, 3); /* B: 4 + 400, two data sectors */
    const slot gone = b.add("GONE", t_binary, body);
    b.remove(gone);
    for (const auto& [t, s] : gone.sectors)
    {
        REQUIRE(b.is_free(t, s));
    }
    const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(b.bytes());
    const auto* g = find(d, "GONE");
    REQUIRE(g != nullptr);
    const dumpfloppy::dos33_undelete u = dumpfloppy::undelete_dos33(d, *g);
    CHECK(u.status == dumpfloppy::dos33_recovery::full);
    CHECK(u.reused == 0u);
    CHECK(u.lists == 1u);
    CHECK(u.data_sectors == 2u);
    CHECK(u.reason.empty());
    REQUIRE(u.bytes.size() == 404u); /* B real length: 4 + length word */
    CHECK(std::equal(body.begin(), body.end(), u.bytes.begin()));
    /* Live entries are not "recovered". */
    CHECK(dumpfloppy::undelete_dos33(d, *find(d, "KEEP")).status ==
          dumpfloppy::dos33_recovery::none);

    const std::string s = report_undel(b.bytes(), true);
    CHECK(line_with(s, "XXH64 Checksum").find("Recovery") != std::string::npos);
    const std::string gl = line_with(s, "GONE");
    CHECK(gl.find("404") != std::string::npos);
    CHECK(gl.ends_with("full"));
    CHECK(line_with(s, "KEEP").find("full") == std::string::npos);

    const auto doc = json_undel(b.bytes(), true);
    const auto* jg = json_entry(doc, "GONE");
    REQUIRE(jg != nullptr);
    CHECK(jg->at("size") == 404);
    CHECK(jg->at("dos33").at("recovered") == true);
    CHECK(jg->at("dos33").at("recovery") == "full");
    CHECK(jg->at("dos33").at("reused_sectors") == 0);
    CHECK(jg->at("dos33").at("recovery_reason").is_null());
    const auto* jk = json_entry(doc, "KEEP");
    REQUIRE(jk != nullptr);
    CHECK(jk->at("dos33").at("recovered") == false);
    CHECK(jk->at("dos33").at("recovery").is_null());

    const auto dest = undel_dest("dos33-undel-full");
    CHECK(extract_undel(b.bytes(), dest, true) == 2);
    CHECK(slurp(dest / "GONE.undeleted") == std::string(body.begin(), body.end()));
    CHECK_FALSE(std::filesystem::exists(dest / "?GONE"));
    CHECK(slurp(dest / "KEEP") == std::string(10, 'k'));
    std::filesystem::remove_all(dest);
}

TEST_CASE("DOS 3.3 --undelete: a reused sector makes the file partial",
          "[apple][dos33][undelete]")
{
    builder b;
    std::vector<uint8_t> text(600, 'o'); /* T: three data sectors, ends at $00 */
    text.push_back(0);
    const slot gone = b.add("OLD", t_text, text);
    b.remove(gone);
    /* A later file took data sector 2 (sectors[0] is the T/S list). */
    const auto [rt, rs] = gone.sectors[2];
    b.set_free(rt, rs, false);
    std::memset(b.bytes().data() + builder::at(rt, rs), 'N', k_sector);

    const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(b.bytes());
    const dumpfloppy::dos33_undelete u = dumpfloppy::undelete_dos33(d, *find(d, "OLD"));
    CHECK(u.status == dumpfloppy::dos33_recovery::partial);
    CHECK(u.reused == 1u);
    CHECK(u.reason.find("1 sector(s) marked used again") != std::string::npos);
    /* Bytes are taken as they are on disk: no guessing. The overwritten
     * sector has no $00, so the T length runs to the next $00. */
    REQUIRE(u.bytes.size() == 600u);
    CHECK(u.bytes[255] == 'o');
    CHECK(u.bytes[256] == 'N');

    const std::string s = report_undel(b.bytes(), true);
    CHECK(line_with(s, " OLD ").ends_with("partial (1 reused)"));
    CHECK(line_with(s, "Recovery note: OLD: partial") != "");

    const auto doc = json_undel(b.bytes(), true);
    const auto* jo = json_entry(doc, "OLD");
    REQUIRE(jo != nullptr);
    CHECK(jo->at("dos33").at("recovered") == true);
    CHECK(jo->at("dos33").at("recovery") == "partial");
    CHECK(jo->at("dos33").at("reused_sectors") == 1);
    CHECK(jo->at("dos33").at("recovery_reason").is_string());

    const auto dest = undel_dest("dos33-undel-partial");
    std::string err;
    CHECK(extract_undel(b.bytes(), dest, true, &err) == 1);
    CHECK(slurp(dest / "OLD.partial").size() == 600u);
    CHECK_FALSE(std::filesystem::exists(dest / "OLD.undeleted"));
    CHECK(err.find("partial recovery") != std::string::npos);
    std::filesystem::remove_all(dest);
}

TEST_CASE("DOS 3.3 --undelete: broken and looping T/S chains stop",
          "[apple][dos33][undelete]")
{
    /* Two lists (123 data sectors); list 2 links back to list 1. */
    std::vector<uint8_t> big(123u * k_sector);
    for (std::size_t i = 0; i < big.size(); ++i)
    {
        big[i] = static_cast<uint8_t>(i / k_sector + 1u);
    }
    SECTION("loop")
    {
        builder b;
        const slot gone = b.add("LOOPY", t_s, big);
        b.remove(gone);
        const auto [l2t, l2s] = gone.sectors[1];
        auto& img = b.bytes();
        img[builder::at(l2t, l2s) + 1u] = gone.ts_track;
        img[builder::at(l2t, l2s) + 2u] = gone.ts_sector;
        const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(img);
        const auto u = dumpfloppy::undelete_dos33(d, *find(d, "LOOPY"));
        CHECK(u.status == dumpfloppy::dos33_recovery::partial);
        CHECK(u.lists == 2u);
        CHECK(u.reused == 0u);
        CHECK(u.reason.find("loops") != std::string::npos);
        CHECK(u.bytes.size() == big.size());
        const auto doc = json_undel(img, true);
        const auto* jl = json_entry(doc, "LOOPY");
        REQUIRE(jl != nullptr);
        CHECK(jl->at("dos33").at("recovery") == "partial");
    }
    SECTION("bad link after the first list")
    {
        builder b;
        const slot gone = b.add("BROKEN", t_s, big);
        b.remove(gone);
        auto& img = b.bytes();
        img[builder::at(gone.ts_track, gone.ts_sector) + 1u] = 0x60; /* track 96 */
        const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(img);
        const auto u = dumpfloppy::undelete_dos33(d, *find(d, "BROKEN"));
        CHECK(u.status == dumpfloppy::dos33_recovery::partial);
        CHECK(u.lists == 1u);
        CHECK(u.reason.find("bad link") != std::string::npos);
        CHECK(u.bytes.size() == 122u * k_sector);
    }
    SECTION("first list outside the volume is unrecoverable")
    {
        builder b;
        b.add("LIVE", t_text, std::vector<uint8_t>(5, 'l'));
        const slot gone = b.add("NOWHERE", t_text, std::vector<uint8_t>(5, 'n'));
        b.remove(gone);
        auto& img = b.bytes();
        img[gone.off + 0x20u] = 0x50; /* old T/S track 80 */
        const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(img);
        const auto u = dumpfloppy::undelete_dos33(d, *find(d, "NOWHERE"));
        CHECK(u.status == dumpfloppy::dos33_recovery::unrecoverable);
        CHECK(u.bytes.empty());
        CHECK(line_with(report_undel(img, true), "NOWHERE").ends_with("unrecoverable"));
        const auto doc = json_undel(img, true);
        const auto* jn = json_entry(doc, "NOWHERE");
        REQUIRE(jn != nullptr);
        CHECK(jn->at("dos33").at("recovered") == false);
        CHECK(jn->at("dos33").at("recovery") == "unrecoverable");
        const auto dest = undel_dest("dos33-undel-none");
        std::string err;
        CHECK(extract_undel(img, dest, true, &err) == 1);
        CHECK(err.find("NOWHERE: not recovered") != std::string::npos);
        CHECK_FALSE(std::filesystem::exists(dest / "NOWHERE.partial"));
        CHECK_FALSE(std::filesystem::exists(dest / "NOWHERE.undeleted"));
        std::filesystem::remove_all(dest);
    }
    SECTION("empty old T/S list is unrecoverable")
    {
        builder b;
        const slot gone = b.add("ZEROED", t_text, std::vector<uint8_t>(5, 'z'));
        b.remove(gone);
        std::memset(b.bytes().data() + builder::at(gone.ts_track, gone.ts_sector), 0, k_sector);
        const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(b.bytes());
        const auto u = dumpfloppy::undelete_dos33(d, *find(d, "ZEROED"));
        CHECK(u.status == dumpfloppy::dos33_recovery::unrecoverable);
        CHECK(u.reason.find("no data sectors") != std::string::npos);
    }
}

TEST_CASE("DOS 3.3 --undelete: multi-sector T/S chain recovers in order",
          "[apple][dos33][undelete]")
{
    builder b;
    std::vector<uint8_t> big(130u * k_sector);
    for (std::size_t i = 0; i < big.size(); ++i)
    {
        big[i] = static_cast<uint8_t>((i * 7u) ^ (i / k_sector));
    }
    const slot gone = b.add("CHAIN", t_new_b, big); /* B2: T/S-list extent */
    b.remove(gone);
    const dumpfloppy::apple_disk d = dumpfloppy::parse_apple(b.bytes());
    const auto u = dumpfloppy::undelete_dos33(d, *find(d, "CHAIN"));
    CHECK(u.status == dumpfloppy::dos33_recovery::full);
    CHECK(u.lists == 2u);
    CHECK(u.data_sectors == 130u);
    CHECK(u.bytes == big);
}

TEST_CASE("DOS 3.3 --undelete never takes a live file's name", "[apple][dos33][undelete]")
{
    builder b;
    b.add("X.undeleted", t_text, std::vector<uint8_t>(4, 'L'));
    const slot gone = b.add("X", t_text, std::vector<uint8_t>(4, 'D'));
    b.remove(gone);
    const auto dest = undel_dest("dos33-undel-live");
    CHECK(extract_undel(b.bytes(), dest, true) == 2);
    CHECK(slurp(dest / "X.undeleted") == "LLLL");
    std::size_t files = 0;
    for (const auto& e : std::filesystem::directory_iterator(dest))
    {
        ++files;
        if (e.path().filename() != "X.undeleted")
        {
            CHECK(slurp(e.path()) == "DDDD");
        }
    }
    CHECK(files == 2u);
    std::filesystem::remove_all(dest);
}

TEST_CASE("DOS 3.3 default output is unchanged without --undelete",
          "[apple][dos33][undelete]")
{
    builder b;
    b.add("KEEP", t_text, std::vector<uint8_t>(10, 'k'));
    const slot gone = b.add("GONE", t_applesoft, with_header({40}, 40, 9));
    b.remove(gone);
    const std::string plain = report_undel(b.bytes(), false);
    CHECK(plain == report_of(b.bytes()));
    CHECK(plain.find("Recovery") == std::string::npos);
    CHECK(line_with(plain, "GONE").find("full") == std::string::npos);
    const auto doc = json_undel(b.bytes(), false);
    const auto* jg = json_entry(doc, "GONE");
    REQUIRE(jg != nullptr);
    for (const char* k : {"recovered", "recovery", "reused_sectors", "recovery_reason"})
    {
        CHECK_FALSE(jg->at("dos33").contains(k));
    }
    CHECK(doc == nlohmann::json::parse(json_of(b.bytes())));
    const auto dest = undel_dest("dos33-undel-default");
    CHECK(extract_undel(b.bytes(), dest, false) == 2);
    CHECK(slurp(dest / "?GONE").size() == 42u);
    CHECK_FALSE(std::filesystem::exists(dest / "GONE.undeleted"));
    std::filesystem::remove_all(dest);
}
