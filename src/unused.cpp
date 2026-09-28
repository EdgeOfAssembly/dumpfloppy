/**
 * @file unused.cpp
 * @brief Recover leftover bytes from FAT-free and FAT-bad clusters.
 *
 * Live files and deleted dirents with a still-allocated chain are listed
 * elsewhere. This walk is the unallocated map: format-fill (`0xF6`) is
 * ignored; consecutive dirty free clusters are one run. Typical cause:
 * files deleted without a full format, then a duplicator copies every
 * sector (Sierra FormMaster / any raw track copy). SQ2 Disk 1 is one
 * published example (AGI C sources + AGI.EXE memory map in FAT-free
 * space); the scanner is not game-specific.
 */
#include "dumpfloppy/unused.hpp"
#include "dumpfloppy/fat.hpp"
#include "dumpfloppy/fat12_codec.h"
#include "dumpfloppy/util.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace dumpfloppy
{
namespace
{

constexpr unsigned k_min_dirty = 8u;
constexpr unsigned k_preview_chars = 48u;

[[nodiscard]] uint32_t cluster_bytes(const bpb_info& bpb) noexcept
{
    return static_cast<uint32_t>(bpb.bytes_per_sector) *
           static_cast<uint32_t>(bpb.sectors_per_cluster);
}

[[nodiscard]] bool is_free_entry(fat_kind kind, uint16_t v) noexcept
{
    if (kind == fat_kind::fat12)
    {
        return fat12_is_free(v) != 0;
    }
    return v == 0u;
}

[[nodiscard]] bool is_bad_entry(fat_kind kind, uint16_t v) noexcept
{
    if (kind == fat_kind::fat12)
    {
        return fat12_is_bad(v) != 0;
    }
    return v == 0xFFF7u;
}

[[nodiscard]] bool cluster_has_leftover(std::span<const uint8_t> bytes) noexcept
{
    if (bytes.size() < k_min_dirty)
    {
        return false;
    }
    unsigned dirty = 0;
    const uint8_t first = bytes[0];
    bool all_same = true;
    for (uint8_t b : bytes)
    {
        if (b != first)
        {
            all_same = false;
        }
        if (b != 0u && b != 0xF6u)
        {
            ++dirty;
        }
    }
    if (all_same && (first == 0u || first == 0xF6u || first == 0xE5u ||
                     first == 0xFFu))
    {
        return false;
    }
    return dirty >= k_min_dirty;
}

[[nodiscard]] bool contains_bytes(std::span<const uint8_t> hay,
                                  std::string_view needle) noexcept
{
    if (needle.empty() || hay.size() < needle.size())
    {
        return false;
    }
    const auto* p = reinterpret_cast<const char*>(hay.data());
    return std::search(p, p + hay.size(), needle.begin(), needle.end()) !=
           p + hay.size();
}

[[nodiscard]] bool mostly_text(std::span<const uint8_t> bytes) noexcept
{
    if (bytes.empty())
    {
        return false;
    }
    unsigned ok = 0;
    for (uint8_t b : bytes)
    {
        if (b == 0x09u || b == 0x0Au || b == 0x0Du ||
            (b >= 0x20u && b <= 0x7Eu))
        {
            ++ok;
        }
    }
    return (static_cast<uint64_t>(ok) * 100u) >=
           (static_cast<uint64_t>(bytes.size()) * 85u);
}

[[nodiscard]] unsigned count_seg_off(std::span<const uint8_t> bytes) noexcept
{
    unsigned hits = 0;
    const size_t n = bytes.size();
    for (size_t i = 0; i + 3u < n; ++i)
    {
        const auto hex = [](uint8_t c) -> bool {
            return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
                   (c >= 'a' && c <= 'f');
        };
        if (bytes[i] != ':')
        {
            continue;
        }
        if (i == 0u || !hex(bytes[i - 1u]) || !hex(bytes[i + 1u]))
        {
            continue;
        }
        ++hits;
        if (hits >= 64u)
        {
            break;
        }
    }
    return hits;
}

[[nodiscard]] bool looks_like_map(std::span<const uint8_t> bytes) noexcept
{
    if (contains_bytes(bytes, "Segment ") && contains_bytes(bytes, "Addr ="))
    {
        return true;
    }
    return count_seg_off(bytes) >= 8u && mostly_text(bytes);
}

[[nodiscard]] size_t skip_text_ws(std::span<const uint8_t> bytes) noexcept
{
    size_t i = 0;
    while (i < bytes.size())
    {
        const uint8_t b = bytes[i];
        if (b != ' ' && b != '\t' && b != '\r' && b != '\n')
        {
            break;
        }
        ++i;
    }
    return i;
}

[[nodiscard]] bool starts_with_ci(std::span<const uint8_t> bytes,
                                  std::string_view word) noexcept
{
    if (bytes.size() < word.size())
    {
        return false;
    }
    for (size_t i = 0; i < word.size(); ++i)
    {
        const unsigned char a = static_cast<unsigned char>(bytes[i]);
        const unsigned char b = static_cast<unsigned char>(word[i]);
        if (std::tolower(a) != std::tolower(b))
        {
            return false;
        }
    }
    return true;
}

/**
 * True when bytes after a DOS Ctrl-Z look like a new text file (C banner,
 * include, ASM comment, or BAT keyword). Binary 0x1A in object leftovers
 * fails this test.
 */
[[nodiscard]] bool looks_like_new_text_file(std::span<const uint8_t> bytes) noexcept
{
    const size_t i = skip_text_ws(bytes);
    if (i >= bytes.size())
    {
        return false;
    }
    const std::span<const uint8_t> s = bytes.subspan(i);
    if (s.size() >= 2u && s[0] == '/' && s[1] == '*')
    {
        return true;
    }
    if (starts_with_ci(s, "#include"))
    {
        return true;
    }
    if (s[0] == ';')
    {
        return true;
    }
    if (starts_with_ci(s, "echo") || starts_with_ci(s, "copy") ||
        starts_with_ci(s, "rem ") || starts_with_ci(s, "set "))
    {
        return true;
    }
    return false;
}

[[nodiscard]] std::string banner_stem(std::span<const uint8_t> bytes)
{
    size_t i = skip_text_ws(bytes);
    if (i + 2u > bytes.size() || bytes[i] != '/' || bytes[i + 1u] != '*')
    {
        return {};
    }
    i += 2u;
    while (i < bytes.size() && (bytes[i] == ' ' || bytes[i] == '\t'))
    {
        ++i;
    }
    if (i >= bytes.size())
    {
        return {};
    }
    const unsigned char first = bytes[i];
    if (!std::isalpha(first) && first != '_')
    {
        return {};
    }
    std::string stem;
    stem.push_back(static_cast<char>(std::toupper(first)));
    ++i;
    while (i < bytes.size() && stem.size() < 12u)
    {
        const unsigned char c = bytes[i];
        if (std::isalnum(c) || c == '_' || c == '.')
        {
            stem.push_back(static_cast<char>(std::toupper(c)));
            ++i;
            continue;
        }
        break;
    }
    return stem;
}

void classify(unused_run& run)
{
    const std::span<const uint8_t> p = run.payload;
    const size_t w = skip_text_ws(p);
    const std::span<const uint8_t> head =
        (w < p.size()) ? p.subspan(w) : std::span<const uint8_t>{};
    if (!head.empty() && head[0] == ';')
    {
        run.guess = "assembly";
        return;
    }
    if (contains_bytes(p, "#include") || contains_bytes(p, "STRPTR") ||
        (head.size() >= 2u && head[0] == '/' && head[1] == '*'))
    {
        run.guess = "C source";
        return;
    }
    if (looks_like_map(p))
    {
        run.guess = "memory map";
        return;
    }
    if (starts_with_ci(head, "echo") || starts_with_ci(head, "copy") ||
        starts_with_ci(head, "rem ") || starts_with_ci(head, "set "))
    {
        run.guess = "batch";
        return;
    }
    if (mostly_text(p))
    {
        run.guess = "text";
        return;
    }
    run.guess = "binary";
}

void set_host_name(unused_run& run)
{
    const char* ext = ".bin";
    if (run.guess == "C source")
    {
        ext = ".c";
    }
    else if (run.guess == "memory map")
    {
        ext = ".map";
    }
    else if (run.guess == "assembly")
    {
        ext = ".asm";
    }
    else if (run.guess == "batch")
    {
        ext = ".bat";
    }
    else if (run.guess == "text")
    {
        ext = ".txt";
    }

    const std::string stem = banner_stem(run.payload);
    char buf[48] = {};
    if (!stem.empty() && stem.find("..") == std::string::npos)
    {
        const size_t dot = stem.rfind('.');
        if (dot != std::string::npos && dot + 1u < stem.size())
        {
            std::snprintf(buf, sizeof(buf), "unused_%s", stem.c_str());
        }
        else
        {
            std::snprintf(buf, sizeof(buf), "unused_%s%s", stem.c_str(), ext);
        }
    }
    else
    {
        std::snprintf(buf, sizeof(buf), "unused_c%04u%s",
                      static_cast<unsigned>(run.first_cluster), ext);
    }
    run.host_name = buf;
}

void uniquify_host_name(unused_run& run, const std::vector<unused_run>& out)
{
    const std::string original = run.host_name;
    const size_t dot = original.rfind('.');
    const std::string stem =
        (dot == std::string::npos) ? original : original.substr(0, dot);
    const std::string ext =
        (dot == std::string::npos) ? std::string{} : original.substr(dot);
    unsigned n = 2;
    for (;;)
    {
        bool taken = false;
        for (const unused_run& other : out)
        {
            if (other.host_name == run.host_name)
            {
                taken = true;
                break;
            }
        }
        if (!taken)
        {
            return;
        }
        run.host_name = stem + "_" + std::to_string(n) + ext;
        ++n;
    }
}

void set_preview(unused_run& run)
{
    std::string out;
    out.reserve(k_preview_chars);
    for (uint8_t b : run.payload)
    {
        if (out.size() >= k_preview_chars)
        {
            break;
        }
        if (b == 0u)
        {
            continue;
        }
        if (b == '\n' || b == '\r' || b == '\t')
        {
            if (!out.empty() && out.back() != ' ')
            {
                out.push_back(' ');
            }
            continue;
        }
        if (b >= 0x20u && b <= 0x7Eu)
        {
            out.push_back(static_cast<char>(b));
        }
    }
    while (!out.empty() && out.back() == ' ')
    {
        out.pop_back();
    }
    run.preview = std::move(out);
}

void trim_trailing_nuls(std::vector<uint8_t>& p)
{
    while (!p.empty() && p.back() == 0u)
    {
        p.pop_back();
    }
}

void emit_run(unused_run run, std::vector<unused_run>& out)
{
    if (run.payload.size() < k_min_dirty)
    {
        return;
    }
    while (!run.payload.empty() && run.payload.back() == 0x1Au)
    {
        run.payload.pop_back();
    }
    if (run.payload.size() < k_min_dirty)
    {
        return;
    }
    classify(run);
    set_host_name(run);
    uniquify_host_name(run, out);
    set_preview(run);
    run.xxh64 = xxh64_hex(run.payload);
    out.push_back(std::move(run));
}

void slice_run(const unused_run& src, size_t begin, size_t end, uint32_t clusz,
               std::vector<unused_run>& out)
{
    if (end <= begin || clusz == 0u)
    {
        return;
    }
    unused_run part = src;
    part.payload.assign(src.payload.begin() + static_cast<std::ptrdiff_t>(begin),
                        src.payload.begin() + static_cast<std::ptrdiff_t>(end));
    trim_trailing_nuls(part.payload);
    const uint32_t delta0 = static_cast<uint32_t>(begin / clusz);
    const uint32_t delta1 =
        static_cast<uint32_t>((end > 0u ? end - 1u : 0u) / clusz);
    part.first_cluster =
        static_cast<uint16_t>(src.first_cluster + static_cast<uint16_t>(delta0));
    part.last_cluster =
        static_cast<uint16_t>(src.first_cluster + static_cast<uint16_t>(delta1));
    emit_run(std::move(part), out);
}

/**
 * Split a mostly-text leftover on DOS Ctrl-Z (0x1A) when the following
 * bytes look like a new text file. Binary 0x1A is left in place.
 */
[[nodiscard]] size_t find_text_file_after_map(std::span<const uint8_t> bytes)
{
    if (bytes.size() < k_min_dirty)
    {
        return static_cast<size_t>(-1);
    }
    std::vector<size_t> cands;
    for (size_t i = 0; i + 1u < bytes.size(); ++i)
    {
        if (bytes[i] == '/' && bytes[i + 1u] == '*')
        {
            cands.push_back(i);
        }
    }
    const char inc[] = "#include";
    const size_t inc_len = sizeof(inc) - 1u;
    for (size_t i = 0; i + inc_len <= bytes.size(); ++i)
    {
        if (std::memcmp(bytes.data() + i, inc, inc_len) == 0)
        {
            cands.push_back(i);
        }
    }
    std::sort(cands.begin(), cands.end());
    cands.erase(std::unique(cands.begin(), cands.end()), cands.end());
    for (size_t cand : cands)
    {
        if (cand == 0u)
        {
            continue;
        }
        const std::span<const uint8_t> head(bytes.data(), cand);
        if (looks_like_map(head) && looks_like_new_text_file(bytes.subspan(cand)))
        {
            return cand;
        }
    }
    return static_cast<size_t>(-1);
}

void emit_ctrlz_split(unused_run run, uint32_t clusz,
                      std::vector<unused_run>& out)
{
    if (!mostly_text(run.payload) && !contains_bytes(run.payload, "#include") &&
        !contains_bytes(run.payload, "/*"))
    {
        emit_run(std::move(run), out);
        return;
    }

    std::vector<size_t> starts;
    starts.push_back(0);
    std::vector<size_t> ends;
    const size_t n = run.payload.size();
    for (size_t i = 0; i < n; ++i)
    {
        if (run.payload[i] != 0x1Au)
        {
            continue;
        }
        const std::span<const uint8_t> rest =
            std::span<const uint8_t>(run.payload).subspan(i + 1u);
        if (!looks_like_new_text_file(rest))
        {
            continue;
        }
        ends.push_back(i);
        starts.push_back(i + 1u);
    }
    ends.push_back(n);
    if (starts.size() != ends.size() || starts.size() <= 1u)
    {
        emit_run(std::move(run), out);
        return;
    }
    for (size_t k = 0; k < starts.size(); ++k)
    {
        slice_run(run, starts[k], ends[k], clusz, out);
    }
}

/**
 * Split a leftover run when a linker map (seg:off columns) is followed by
 * C sources (`#include`). Then split text leftovers on DOS Ctrl-Z.
 */
void finish_run(unused_run& run, uint32_t clusz, std::vector<unused_run>& out)
{
    if (run.payload.empty())
    {
        return;
    }
    trim_trailing_nuls(run.payload);
    if (run.payload.size() < k_min_dirty)
    {
        return;
    }

    const size_t split = find_text_file_after_map(run.payload);
    if (split != static_cast<size_t>(-1) && clusz != 0u)
    {
        unused_run map = run;
        map.last_cluster = static_cast<uint16_t>(
            run.first_cluster + static_cast<uint16_t>((split - 1u) / clusz));
        map.payload.assign(run.payload.begin(),
                           run.payload.begin() +
                               static_cast<std::ptrdiff_t>(split));
        trim_trailing_nuls(map.payload);
        emit_run(std::move(map), out);

        unused_run src = run;
        const uint32_t delta = static_cast<uint32_t>(split / clusz);
        src.first_cluster = static_cast<uint16_t>(run.first_cluster + delta);
        src.payload.assign(run.payload.begin() +
                               static_cast<std::ptrdiff_t>(split),
                           run.payload.end());
        emit_ctrlz_split(std::move(src), clusz, out);
        return;
    }
    emit_ctrlz_split(std::move(run), clusz, out);
}

} /* namespace */

std::vector<unused_run> scan_unused_clusters(std::span<const uint8_t> volume,
                                             const bpb_info& bpb, fat_kind kind)
{
    std::vector<unused_run> out;
    if (!bpb.looks_valid || (kind != fat_kind::fat12 && kind != fat_kind::fat16) ||
        bpb.bytes_per_sector == 0u || bpb.sectors_per_cluster == 0u)
    {
        return out;
    }

    const uint32_t clusz = cluster_bytes(bpb);
    if (clusz == 0u)
    {
        return out;
    }
    const uint32_t nclus = data_cluster_count(bpb);
    if (nclus == 0u)
    {
        return out;
    }
    const uint32_t max_cluster = 1u + nclus;
    const size_t fat0_off =
        static_cast<size_t>(bpb.reserved_sectors) * bpb.bytes_per_sector;
    const size_t fat_bytes =
        static_cast<size_t>(bpb.sectors_per_fat_16) * bpb.bytes_per_sector;
    if (fat_bytes == 0u || fat0_off + fat_bytes > volume.size())
    {
        return out;
    }
    const std::span<const uint8_t> fat0 = volume.subspan(fat0_off, fat_bytes);

    unused_run cur{};
    bool open = false;

    for (uint32_t c = 2; c <= max_cluster; ++c)
    {
        uint16_t v = 0;
        if (!fat_get(fat0, kind, c, v))
        {
            break;
        }
        unused_kind k = unused_kind::fat_free;
        if (is_free_entry(kind, v))
        {
            k = unused_kind::fat_free;
        }
        else if (is_bad_entry(kind, v))
        {
            k = unused_kind::fat_bad;
        }
        else
        {
            if (open)
            {
                finish_run(cur, clusz, out);
                open = false;
                cur = unused_run{};
            }
            continue;
        }

        const size_t off = cluster_offset(bpb, c);
        if (off == static_cast<size_t>(-1) || off + clusz > volume.size())
        {
            if (open)
            {
                finish_run(cur, clusz, out);
                open = false;
                cur = unused_run{};
            }
            continue;
        }
        const std::span<const uint8_t> chunk = volume.subspan(off, clusz);
        if (!cluster_has_leftover(chunk))
        {
            if (open)
            {
                finish_run(cur, clusz, out);
                open = false;
                cur = unused_run{};
            }
            continue;
        }

        if (open && cur.kind == k &&
            static_cast<uint32_t>(cur.last_cluster) + 1u == c)
        {
            cur.last_cluster = static_cast<uint16_t>(c);
            cur.payload.insert(cur.payload.end(), chunk.begin(), chunk.end());
            continue;
        }
        if (open)
        {
            finish_run(cur, clusz, out);
        }
        cur = unused_run{};
        cur.first_cluster = static_cast<uint16_t>(c);
        cur.last_cluster = static_cast<uint16_t>(c);
        cur.kind = k;
        cur.payload.assign(chunk.begin(), chunk.end());
        open = true;
    }
    if (open)
    {
        finish_run(cur, clusz, out);
    }
    return out;
}

} /* namespace dumpfloppy */
