/**
 * @file test_update.cpp
 * @brief FAT replace: same size, shrink, grow, relocate, reclaim, disk-full, MFM DAM.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/cli.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/fat.hpp"
#include "dumpfloppy/fat12_codec.h"
#include "dumpfloppy/ibm_mfm.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/update.hpp"
#include "image_builder.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

std::filesystem::path write_temp(const std::vector<uint8_t>& bytes,
                                 const std::string& name)
{
    const auto dir = std::filesystem::temp_directory_path() / "dumpfloppy-tests";
    std::filesystem::create_directories(dir);
    const auto path = dir / name;
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    REQUIRE(out);
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    REQUIRE(out);
    return path;
}

const dumpfloppy::dir_entry* find_name(const dumpfloppy::analysis& a,
                                       const std::string& name)
{
    for (const dumpfloppy::dir_entry& e : a.entries)
    {
        if (e.name_83 == name && !e.deleted)
        {
            return &e;
        }
    }
    return nullptr;
}

} /* namespace */

TEST_CASE("parse_cli -u / --update / --update=", "[cli][update]")
{
    auto parse = [](const std::vector<std::string>& args)
    {
        std::vector<std::string> storage;
        storage.push_back("dumpfloppy");
        storage.insert(storage.end(), args.begin(), args.end());
        std::vector<char*> ptrs;
        ptrs.reserve(storage.size());
        for (std::string& s : storage)
        {
            ptrs.push_back(s.data());
        }
        return dumpfloppy::parse_cli(static_cast<int>(ptrs.size()), ptrs.data());
    };

    const auto a = parse({"disk.ima", "-u", "PENGUIN.EXE"});
    REQUIRE(a.ok);
    REQUIRE(a.update.enabled);
    REQUIRE(a.update.hosts.size() == 1);
    REQUIRE(a.update.hosts[0] == "PENGUIN.EXE");
    REQUIRE(a.inputs.size() == 1);

    const auto b = parse({"--update=HELLO.TXT", "disk.ima", "-u", "FILEB.TXT"});
    REQUIRE(b.ok);
    REQUIRE(b.update.hosts.size() == 2);
    REQUIRE(b.update.hosts[0] == "HELLO.TXT");
    REQUIRE(b.update.hosts[1] == "FILEB.TXT");

    const auto c = parse({"-u"});
    REQUIRE_FALSE(c.ok);
    REQUIRE(c.error.find("missing FILE") != std::string::npos);

    const std::string u = dumpfloppy::usage_text();
    REQUIRE(u.find("-u, --update") != std::string::npos);
}

TEST_CASE("update same-size overwrites HELLO.TXT in place", "[update]")
{
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    const auto img = write_temp(bytes, "upd-same.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    const auto* hello = find_name(a, "HELLO.TXT");
    REQUIRE(hello != nullptr);
    const uint16_t cl = hello->first_cluster;
    REQUIRE(cl == 2);

    const auto host = write_temp(
        std::vector<uint8_t>{'N', 'e', 'w', ' ', 'p', 'a', 'y', 'l', 'o', 'a', 'd', '!', '\n', 'X'},
        "HELLO.TXT");
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.push_back(host);
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == 1);
    REQUIRE(err.str().empty());

    hello = find_name(a, "HELLO.TXT");
    REQUIRE(hello != nullptr);
    REQUIRE(hello->size == 14);
    REQUIRE(hello->first_cluster == cl);
    const std::vector<uint8_t> got =
        dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *hello);
    REQUIRE(got.size() == 14);
    REQUIRE(std::string(got.begin(), got.end()) == "New payload!\nX");

    const dumpfloppy::fat_summary fat =
        dumpfloppy::summarise_fat(a.image.bytes, a.bpb, a.kind);
    REQUIRE(fat.copies_match);
}

TEST_CASE("update shrink frees the tail cluster", "[update]")
{
    auto bytes = dumpfloppy_test::make_fat12_packed();
    const auto img = write_temp(bytes, "upd-shrink.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    const auto* filea = find_name(a, "FILEA.TXT");
    REQUIRE(filea != nullptr);
    REQUIRE(filea->cluster_chain.size() == 2);

    const auto host = write_temp(std::vector<uint8_t>{'x', 'y', '\n'}, "FILEA.TXT");
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.push_back(host);
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == 1);
    REQUIRE(err.str().empty());

    filea = find_name(a, "FILEA.TXT");
    REQUIRE(filea != nullptr);
    REQUIRE(filea->size == 3);
    REQUIRE(filea->cluster_chain.size() == 1);
    const auto* fileb = find_name(a, "FILEB.TXT");
    REQUIRE(fileb != nullptr);
    REQUIRE(fileb->first_cluster == 4);
    const std::vector<uint8_t> body =
        dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *filea);
    REQUIRE(std::string(body.begin(), body.end()) == "xy\n");
}

TEST_CASE("update grow allocates a free cluster around a deleted file", "[update]")
{
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    const auto img = write_temp(bytes, "upd-grow.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));

    std::vector<uint8_t> big(600, static_cast<uint8_t>('G'));
    const auto host = write_temp(big, "HELLO.TXT");
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.push_back(host);
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == 1);
    REQUIRE(err.str().empty());

    const auto* hello = find_name(a, "HELLO.TXT");
    REQUIRE(hello != nullptr);
    REQUIRE(hello->size == 600);
    REQUIRE(hello->cluster_chain.size() == 2);
    REQUIRE(hello->cluster_chain[0] == 2);
    REQUIRE(hello->cluster_chain[1] != 3); /* cluster 3 still holds deleted GONE.TXT */
    const std::vector<uint8_t> got =
        dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *hello);
    REQUIRE(got == big);
}

TEST_CASE("update grow relocates the next live file", "[update]")
{
    const auto bytes = dumpfloppy_test::make_fat12_packed();
    const auto img = write_temp(bytes, "upd-reloc.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    const auto* fileb = find_name(a, "FILEB.TXT");
    REQUIRE(fileb != nullptr);
    REQUIRE(fileb->first_cluster == 4);

    std::vector<uint8_t> big(1200, static_cast<uint8_t>('Z'));
    const auto host = write_temp(big, "FILEA.TXT");
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.push_back(host);
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == 1);
    REQUIRE(err.str().empty());

    const auto* filea = find_name(a, "FILEA.TXT");
    REQUIRE(filea != nullptr);
    REQUIRE(filea->size == 1200);
    REQUIRE(filea->cluster_chain.size() == 3);
    REQUIRE(filea->cluster_chain[0] == 2);
    REQUIRE(filea->cluster_chain[1] == 3);
    REQUIRE(filea->cluster_chain[2] == 4);

    fileb = find_name(a, "FILEB.TXT");
    REQUIRE(fileb != nullptr);
    REQUIRE(fileb->first_cluster != 4);
    REQUIRE(fileb->size == 10);
    const std::vector<uint8_t> bbody =
        dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *fileb);
    REQUIRE(std::string(bbody.begin(), bbody.end()) == "FILEB-DATA");
    const std::vector<uint8_t> abody =
        dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *filea);
    REQUIRE(abody == big);

    const dumpfloppy::fat_summary fat =
        dumpfloppy::summarise_fat(a.image.bytes, a.bpb, a.kind);
    REQUIRE(fat.copies_match);
}

TEST_CASE("update fails when the volume has no free clusters", "[update]")
{
    const auto bytes = dumpfloppy_test::make_fat12_full();
    const auto img = write_temp(bytes, "upd-full.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    const auto* hello = find_name(a, "HELLO.TXT");
    REQUIRE(hello != nullptr);
    REQUIRE(hello->size == 60u * 512u);

    std::vector<uint8_t> bigger(hello->size + 1u, static_cast<uint8_t>('Q'));
    const auto host = write_temp(bigger, "HELLO.TXT");
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.push_back(host);
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == -1);
    REQUIRE(err.str().find("not enough free") != std::string::npos);
}

TEST_CASE("update reclaim does not steal a live TACTICS chain", "[update][reclaim]")
{
    const auto bytes = dumpfloppy_test::make_fat12_tactics_reuse();
    const auto img = write_temp(bytes, "upd-tactics.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    const auto* tactics = find_name(a, "TACTICS.PKG");
    REQUIRE(tactics != nullptr);
    REQUIRE(tactics->first_cluster == 2);
    REQUIRE(tactics->cluster_chain.size() == 2);
    const std::vector<uint8_t> live = dumpfloppy_test::tactics_live_bytes();
    REQUIRE(dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *tactics) == live);

    bool saw_deleted_reuse = false;
    for (const dumpfloppy::dir_entry& e : a.entries)
    {
        if (e.deleted && e.first_cluster == 2)
        {
            saw_deleted_reuse = true;
            REQUIRE(e.cluster_chain.empty()); /* first cluster is live-owned */
        }
    }
    REQUIRE(saw_deleted_reuse);

    std::vector<uint8_t> big(600, static_cast<uint8_t>('G'));
    const auto host = write_temp(big, "HELLO.TXT");
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.push_back(host);
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == -1);
    REQUIRE(err.str().find("not enough free") != std::string::npos);

    tactics = find_name(a, "TACTICS.PKG");
    REQUIRE(tactics != nullptr);
    REQUIRE(tactics->first_cluster == 2);
    REQUIRE(tactics->cluster_chain.size() == 2);
    REQUIRE(tactics->cluster_chain[0] == 2);
    REQUIRE(tactics->cluster_chain[1] == 3);
    REQUIRE(dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *tactics) == live);

    uint8_t* fat0 = a.image.bytes.data() + dumpfloppy_test::k_bps;
    uint16_t v2 = 0;
    uint16_t v3 = 0;
    REQUIRE(fat12_entry_get(fat0, dumpfloppy_test::k_bps, 2, &v2) == 0);
    REQUIRE(fat12_entry_get(fat0, dumpfloppy_test::k_bps, 3, &v3) == 0);
    REQUIRE(v2 == 3);
    REQUIRE(fat12_is_eof(v3) != 0);

    const auto* hello = find_name(a, "HELLO.TXT");
    REQUIRE(hello != nullptr);
    REQUIRE(hello->first_cluster == 61);
    REQUIRE(hello->size == 14);
}

TEST_CASE("update reclaim frees a deleted chain that is not live-owned", "[update][reclaim]")
{
    const auto bytes = dumpfloppy_test::make_fat12_orphan_tight();
    const auto img = write_temp(bytes, "upd-orphan.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));

    std::vector<uint8_t> big(600, static_cast<uint8_t>('G'));
    const auto host = write_temp(big, "HELLO.TXT");
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.push_back(host);
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == 1);
    REQUIRE(err.str().empty());

    const auto* hello = find_name(a, "HELLO.TXT");
    REQUIRE(hello != nullptr);
    REQUIRE(hello->size == 600);
    REQUIRE(hello->cluster_chain.size() == 2);
    REQUIRE(hello->cluster_chain[0] == 2);
    REQUIRE(hello->cluster_chain[1] == 3);
    REQUIRE(dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *hello) == big);

    const dumpfloppy::fat_summary fat =
        dumpfloppy::summarise_fat(a.image.bytes, a.bpb, a.kind);
    REQUIRE(fat.copies_match);
}

TEST_CASE("update aborts when a neighbour cannot be relocated", "[update][relocate]")
{
    const auto bytes = dumpfloppy_test::make_fat12_relocate_tight();
    const auto img = write_temp(bytes, "upd-reloc-fail.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    const auto* fileb = find_name(a, "FILEB.TXT");
    REQUIRE(fileb != nullptr);
    REQUIRE(fileb->first_cluster == 3);
    REQUIRE(fileb->cluster_chain.size() == 10);

    std::vector<uint8_t> big(600, static_cast<uint8_t>('Z'));
    const auto host = write_temp(big, "FILEA.TXT");
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.push_back(host);
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == -1);
    REQUIRE(err.str().find("relocate") != std::string::npos);

    const auto* filea = find_name(a, "FILEA.TXT");
    REQUIRE(filea != nullptr);
    REQUIRE(filea->first_cluster == 2);
    REQUIRE(filea->size == 10);
    REQUIRE(dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *filea) ==
            (std::vector<uint8_t>{'F', 'I', 'L', 'E', 'A', '-', 'D', 'A', 'T', 'A'}));

    fileb = find_name(a, "FILEB.TXT");
    REQUIRE(fileb != nullptr);
    REQUIRE(fileb->first_cluster == 3);
    REQUIRE(fileb->cluster_chain.size() == 10);
    REQUIRE(dumpfloppy::read_file_contents(a.image.bytes, a.bpb, *fileb) ==
            (std::vector<uint8_t>{'F', 'I', 'L', 'E', 'B', '-', 'D', 'A', 'T', 'A'}));
}

TEST_CASE("update missing name restores the volume", "[update]")
{
    const auto bytes = dumpfloppy_test::make_fat12_sample();
    const auto img = write_temp(bytes, "upd-miss.ima");
    auto loaded = dumpfloppy::load_image(img);
    REQUIRE(loaded);
    dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    const auto host = write_temp(std::vector<uint8_t>{1, 2, 3}, "NOPE.COM");
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.push_back(host);
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == -1);
    REQUIRE(err.str().find("no live file") != std::string::npos);
}

TEST_CASE("patch_mfm_chs encodes a changed 512-byte DAM", "[update][mfm]")
{
    constexpr size_t k_track = 2048;
    std::vector<uint8_t> mfm(k_track, 0);
    dumpfloppy::flux_disk flux{};
    flux.chs_cyls = 1;
    flux.chs_heads = 1;
    flux.chs_spt = 1;
    flux.assembled_chs.assign(512, 0x11);
    dumpfloppy::ibm_sector s{};
    s.cyl = 0;
    s.head = 0;
    s.sector = 1;
    s.bytes = 512;
    s.size_code = 2;
    s.has_dam = true;
    s.dam_mark = 0xFB;
    s.track_file_off = 0;
    s.track_byte_len = k_track;
    s.dam_bit_off = 0;
    s.data.assign(512, 0x11);
    flux.sectors.push_back(s);

    std::vector<uint8_t> neu(512, 0x22);
    REQUIRE(dumpfloppy::patch_mfm_chs(mfm, flux, neu));

    /* Decode MFM data bits (odd positions) and check the first payload byte. */
    uint8_t first = 0;
    for (int b = 0; b < 8; ++b)
    {
        const size_t bit = static_cast<size_t>(b) * 2u + 1u;
        const size_t byte_i = bit / 8u;
        const unsigned shift = static_cast<unsigned>(7u - (bit % 8u));
        const uint8_t data = static_cast<uint8_t>((mfm[byte_i] >> shift) & 1u);
        first = static_cast<uint8_t>((first << 1) | data);
    }
    REQUIRE(first == 0x22);
}

TEST_CASE("update PENGUIN.EXE same-size on Batman MFM", "[update][batman]")
{
    const std::filesystem::path img{
        "/tmp/Batman - The Caped Crusader (1989) (Data East USA, Inc.) (360K) [cp] [!]/"
        "Batman - The Caped Crusader (1989) (Data East USA, Inc.) (360K) (Disk 1) [cp] [!].mfm"};
    if (!std::filesystem::exists(img))
    {
        SKIP("Batman Disk 1 .mfm is not present");
    }
    const auto work = std::filesystem::temp_directory_path() / "dumpfloppy-tests" /
                      "batman-upd.mfm";
    std::filesystem::copy_file(img, work, std::filesystem::copy_options::overwrite_existing);

    auto loaded = dumpfloppy::load_image(work);
    REQUIRE(loaded);
    dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
    const auto* penguin = find_name(a, "PENGUIN.EXE");
    REQUIRE(penguin != nullptr);
    REQUIRE(penguin->size > 2);

    std::vector<uint8_t> payload =
        dumpfloppy::read_file_contents(a.flux.assembled_chs, a.bpb, *penguin);
    REQUIRE(payload.size() == penguin->size);
    REQUIRE(payload[0] == 'M');
    REQUIRE(payload[1] == 'Z');
    payload[2] = static_cast<uint8_t>(payload[2] ^ 0x5Au);

    const auto host = write_temp(payload, "PENGUIN.EXE");
    dumpfloppy::update_options opt{};
    opt.enabled = true;
    opt.hosts.push_back(host);
    std::ostringstream err;
    REQUIRE(dumpfloppy::update_files(a, opt, err) == 1);
    REQUIRE(err.str().empty());

    std::vector<uint8_t> mfm = a.image.bytes;
    REQUIRE(dumpfloppy::patch_mfm_chs(mfm, a.flux, a.flux.assembled_chs));
    std::ofstream out(work, std::ios::binary | std::ios::trunc);
    REQUIRE(out);
    out.write(reinterpret_cast<const char*>(mfm.data()),
              static_cast<std::streamsize>(mfm.size()));
    REQUIRE(out);

    auto reloaded = dumpfloppy::load_image(work);
    REQUIRE(reloaded);
    const dumpfloppy::analysis b = dumpfloppy::analyse(std::move(*reloaded));
    const auto* again = find_name(b, "PENGUIN.EXE");
    REQUIRE(again != nullptr);
    const std::vector<uint8_t> got =
        dumpfloppy::read_file_contents(b.flux.assembled_chs, b.bpb, *again);
    REQUIRE(got == payload);
}
