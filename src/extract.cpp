/**
 * @file extract.cpp
 * @brief Cluster-walk extract of 8.3 files, including deleted names.
 */
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/amiga.hpp"
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/apple.hpp"
#include "dumpfloppy/cbm.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/trd.hpp"
#include "dumpfloppy/util.hpp"
#include "dumpfloppy/volume.hpp"

#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>
#include <unordered_set>
#include <vector>

namespace dumpfloppy
{
namespace
{

std::string fat_host_string(const dir_entry& e)
{
    std::string rel = e.path.empty() ? e.name_83 : e.path;
    for (char& c : rel)
    {
        if (c == '\\')
        {
            c = '/';
        }
    }
    return rel;
}

/** @brief Bytes outside printable ASCII become `_`. `/` and `\\` stay. */
std::string mask_nonprintable(std::string name)
{
    for (char& ch : name)
    {
        const auto u = static_cast<unsigned char>(ch);
        if (u < 0x20u || u > 0x7Eu)
        {
            ch = '_';
        }
    }
    return name;
}

/** @brief Flatten separators that survived the host-name function. */
std::string flatten_separators(std::string name)
{
    for (char& ch : name)
    {
        if (ch == '/' || ch == '\\')
        {
            ch = '_';
        }
    }
    return name;
}

/**
 * @brief True when @p rel cannot escape the extract destination directory.
 *
 * `std::filesystem::path` operator/ replaces the left-hand side when the
 * right-hand side is absolute, so LFNs like `/etc/passwd` must be rejected
 * before join. Empty, `.`, and `..` components are also rejected.
 */
bool path_is_safe(const std::filesystem::path& rel)
{
    if (rel.empty() || rel.is_absolute() || rel.has_root_name() ||
        rel.has_root_directory())
    {
        return false;
    }

    const std::string raw = rel.generic_string();
    if (raw.empty() || raw[0] == '/' || raw[0] == '\\')
    {
        return false;
    }
    /* Windows drive / UNC even when this host is POSIX. */
    if (raw.size() >= 2u &&
        ((raw[0] >= 'A' && raw[0] <= 'Z') || (raw[0] >= 'a' && raw[0] <= 'z')) &&
        raw[1] == ':')
    {
        return false;
    }
    if (raw.starts_with("//") || raw.starts_with("\\\\"))
    {
        return false;
    }

    auto forbidden = [](std::string_view s) -> bool {
        return s.empty() || s == "." || s == "..";
    };

    std::string part;
    for (char c : raw)
    {
        if (c == '/' || c == '\\')
        {
            if (forbidden(part))
            {
                return false;
            }
            part.clear();
        }
        else
        {
            part.push_back(c);
        }
    }
    if (forbidden(part))
    {
        return false;
    }

    for (const auto& p : rel)
    {
        const std::string s = p.generic_string();
        if (forbidden(s) || s == "/" || s == "\\")
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief One relative leaf, or empty when the name must not be created.
 *
 * Non-printables are masked before path_is_safe so a dirent of 0xFF
 * bytes cannot reach the host filesystem. `/` and `\` stay until that
 * check rejects absolute paths, empty components, `.`, and `..`, then
 * they are flattened. A printable `?` (deleted 8.3 marker) is kept.
 * Amiga, CBM, TRD, and Apple use this. FAT uses @c host_fat_path.
 */
std::string host_leaf(std::string host, std::ostream& err)
{
    host = mask_nonprintable(std::move(host));
    if (!path_is_safe(std::filesystem::path(host)))
    {
        err << "dumpfloppy: skip unsafe path '" << host << "'\n";
        return {};
    }
    host = flatten_separators(std::move(host));
    if (host.empty() || !path_is_safe(std::filesystem::path(host)))
    {
        err << "dumpfloppy: skip unsafe path '" << host << "'\n";
        return {};
    }
    return host;
}

/** @brief FNV-1a 32-bit (offset basis 2166136261, prime 16777619). */
uint32_t fnv1a_32(std::string_view bytes)
{
    uint32_t hash = 2166136261u;
    for (const char ch : bytes)
    {
        hash ^= static_cast<uint32_t>(static_cast<unsigned char>(ch));
        hash *= 16777619u;
    }
    return hash;
}

void append_hex8(std::string& out, uint32_t value)
{
    static constexpr char k_hex[] = "0123456789abcdef";
    char buf[8];
    for (int i = 7; i >= 0; --i)
    {
        buf[i] = k_hex[value & 0x0Fu];
        value >>= 4;
    }
    out.append(buf, 8u);
}

/**
 * @brief Leave a component of at most 255 bytes unchanged.
 *
 * A longer component (already masked to printable ASCII) becomes a
 * 240-byte prefix, `_`, and 8 lowercase hex digits of FNV-1a of that
 * full component, so the host name fits in NAME_MAX and still extracts.
 */
std::string shorten_component(std::string_view comp)
{
    constexpr std::size_t k_max = 255u;
    constexpr std::size_t k_prefix = 240u;
    if (comp.size() <= k_max)
    {
        return std::string(comp);
    }
    std::string out;
    out.reserve(k_prefix + 1u + 8u);
    out.append(comp.substr(0, k_prefix));
    out.push_back('_');
    append_hex8(out, fnv1a_32(comp));
    return out;
}

/** @brief Shorten each `/` or `\\` piece; separators themselves stay. */
std::string shorten_fat_components(std::string host)
{
    std::string out;
    out.reserve(host.size());
    std::string part;
    auto flush = [&]()
    {
        out += shorten_component(part);
        part.clear();
    };
    for (const char c : host)
    {
        if (c == '/' || c == '\\')
        {
            flush();
            out.push_back(c);
        }
        else
        {
            part.push_back(c);
        }
    }
    flush();
    return out;
}

/**
 * @brief Relative FAT path, or empty when the name must not be created.
 *
 * Same rejects as host_leaf (absolute, empty, `.`, `..`) after
 * non-printables are masked. Separators are kept so
 * `RAMTEST/MANUAL.RT` is created under that subdirectory.
 */
std::string host_fat_path(std::string host, std::ostream& err)
{
    host = mask_nonprintable(std::move(host));
    if (!path_is_safe(std::filesystem::path(host)))
    {
        err << "dumpfloppy: skip unsafe path '" << host << "'\n";
        return {};
    }
    host = shorten_fat_components(std::move(host));
    if (host.empty() || !path_is_safe(std::filesystem::path(host)))
    {
        err << "dumpfloppy: skip unsafe path '" << host << "'\n";
        return {};
    }
    return host;
}

std::string dest_key(const std::filesystem::path& p)
{
    return p.lexically_normal().generic_string();
}

bool dest_taken(const std::filesystem::path& p,
                const std::unordered_set<std::string>& used)
{
    if (used.contains(dest_key(p)))
    {
        return true;
    }
    std::error_code ec{};
    const std::filesystem::file_status st = std::filesystem::symlink_status(p, ec);
    if (ec)
    {
        return false;
    }
    /* symlink_status does not follow. A dangling link is still present. */
    const std::filesystem::file_type ty = st.type();
    return ty != std::filesystem::file_type::not_found &&
           ty != std::filesystem::file_type::none;
}

/**
 * @brief Canonical destination, following symlinks the user named with `-o`.
 *
 * The longest existing prefix is resolved with `canonical`. A dangling
 * symlink in that prefix is left unchanged so a later check still refuses
 * to create through it. Components that do not exist yet are appended
 * after the resolved prefix and are created as real directories.
 *
 * @param[in] dest User-supplied extract directory.
 * @return Absolute directory to write into, or @p dest when resolution fails.
 */
std::filesystem::path resolve_user_dest(const std::filesystem::path& dest)
{
    std::error_code ec;
    const std::filesystem::path abs = std::filesystem::absolute(dest, ec);
    if (ec)
    {
        return dest;
    }
    std::filesystem::path prefix;
    std::vector<std::filesystem::path> missing;
    for (const std::filesystem::path& part : abs)
    {
        if (!missing.empty())
        {
            missing.push_back(part);
            continue;
        }
        const std::filesystem::path next = prefix / part;
        std::error_code st_ec;
        const std::filesystem::file_status st = std::filesystem::symlink_status(next, st_ec);
        if (st_ec || st.type() == std::filesystem::file_type::not_found ||
            st.type() == std::filesystem::file_type::none)
        {
            missing.push_back(part);
            continue;
        }
        prefix = next;
    }
    if (prefix.empty())
    {
        return dest;
    }
    const std::filesystem::path canon = std::filesystem::canonical(prefix, ec);
    if (ec)
    {
        return dest;
    }
    std::filesystem::path out = canon;
    for (const std::filesystem::path& part : missing)
    {
        out /= part;
    }
    return out;
}

/**
 * @brief True when any existing component of @p path is a symlink.
 *
 * Missing components end the walk (parents may still be created). The link
 * is not followed.
 */
bool has_symlink_component(const std::filesystem::path& path)
{
    std::filesystem::path cur;
    for (const std::filesystem::path& part : path)
    {
        if (part.empty() || part == ".")
        {
            continue;
        }
        cur /= part;
        std::error_code ec{};
        const std::filesystem::file_status st = std::filesystem::symlink_status(cur, ec);
        if (ec || st.type() == std::filesystem::file_type::not_found)
        {
            return false;
        }
        if (std::filesystem::is_symlink(st))
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Create missing parents with mkdir. Do not follow symlinks.
 *
 * An existing real directory is kept. A symlink component is not an error
 * here; the caller skips the file. A non-directory existing component fails.
 *
 * @return 0 when the directory exists or was created, or when a symlink
 *         blocked creation. -1 when mkdir fails.
 */
int ensure_real_directory(const std::filesystem::path& dir, std::ostream& err)
{
    if (dir.empty())
    {
        return 0;
    }
    std::filesystem::path cur;
    for (const std::filesystem::path& part : dir)
    {
        if (part.empty() || part == ".")
        {
            continue;
        }
        cur /= part;
        std::error_code ec{};
        const std::filesystem::file_status st = std::filesystem::symlink_status(cur, ec);
        if (!ec && std::filesystem::is_symlink(st))
        {
            return 0;
        }
        if (!ec && std::filesystem::is_directory(st))
        {
            continue;
        }
        if (!ec && st.type() != std::filesystem::file_type::not_found &&
            st.type() != std::filesystem::file_type::none)
        {
            err << "dumpfloppy: cannot create '" << cur.string() << "'\n";
            return -1;
        }
        if (::mkdir(cur.c_str(), 0755) != 0)
        {
            if (errno == EEXIST)
            {
                std::error_code ec2{};
                const std::filesystem::file_status st2 =
                    std::filesystem::symlink_status(cur, ec2);
                if (!ec2 && std::filesystem::is_symlink(st2))
                {
                    return 0;
                }
                if (!ec2 && std::filesystem::is_directory(st2))
                {
                    continue;
                }
            }
            err << "dumpfloppy: cannot create '" << cur.string() << "': "
                << std::generic_category().message(errno) << '\n';
            return -1;
        }
    }
    return 0;
}

/**
 * @brief Create @p dest with O_CREAT|O_EXCL|O_NOFOLLOW and write @p bytes.
 *
 * A symlink component is skipped (return 0) and not truncated. Hard errors
 * return -1. Success returns 1.
 */
int write_exclusive_bytes(const std::filesystem::path& dest,
                          const std::vector<uint8_t>& bytes, std::ostream& err)
{
    if (has_symlink_component(dest))
    {
        err << "dumpfloppy: skip symlink path '" << dest.string() << "'\n";
        return 0;
    }
    if (dest.has_parent_path())
    {
        if (ensure_real_directory(dest.parent_path(), err) != 0)
        {
            return -1;
        }
        if (has_symlink_component(dest))
        {
            err << "dumpfloppy: skip symlink path '" << dest.string() << "'\n";
            return 0;
        }
    }

    const int fd = ::open(dest.c_str(), O_CREAT | O_EXCL | O_NOFOLLOW | O_WRONLY, 0644);
    if (fd < 0)
    {
        std::error_code ec{};
        const std::filesystem::file_status st = std::filesystem::symlink_status(dest, ec);
        if (!ec && std::filesystem::is_symlink(st))
        {
            err << "dumpfloppy: skip symlink path '" << dest.string() << "'\n";
            return 0;
        }
        err << "dumpfloppy: cannot write '" << dest.string() << "'\n";
        return -1;
    }

    size_t off = 0;
    bool ok = true;
    while (off < bytes.size())
    {
        const size_t remain = bytes.size() - off;
        const ssize_t n = ::write(fd, bytes.data() + off, remain);
        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            ok = false;
            break;
        }
        if (n == 0)
        {
            ok = false;
            break;
        }
        off += static_cast<size_t>(n);
    }
    if (::close(fd) != 0)
    {
        ok = false;
    }
    if (!ok)
    {
        err << "dumpfloppy: short write '" << dest.string() << "'\n";
        return -1;
    }
    return 1;
}

/**
 * @brief Host path for @p preferred when that path is already taken.
 *
 * A distinct @p fallback_name (a deleted 8.3 such as `?ACTICS.PKG`) is
 * kept and is not rewritten to `.dup`. Live names that still collide
 * become `stem.dup` plus the extension (`FOO.dup.TXT`), then
 * `stem.dup.N` (`FOO.dup.2.TXT`). The warning names the path written.
 *
 * @param[in]  preferred      First-choice host path.
 * @param[in]  fallback_name  Alternate leaf, or empty.
 * @param[in]  used           Destination keys already claimed.
 * @param[out] err            Collision warning.
 *
 * @return A free path, or empty when every candidate is taken.
 */
std::filesystem::path choose_extract_dest(const std::filesystem::path& preferred,
                                          std::string_view fallback_name,
                                          const std::unordered_set<std::string>& used,
                                          std::ostream& err)
{
    if (!dest_taken(preferred, used))
    {
        return preferred;
    }

    const std::filesystem::path parent = preferred.parent_path();
    auto in_parent = [&](const std::string& name) -> std::filesystem::path {
        return parent.empty() ? std::filesystem::path(name) : parent / name;
    };

    if (!fallback_name.empty())
    {
        const std::string fb(fallback_name);
        const std::filesystem::path as_fb = in_parent(fb);
        if (path_is_safe(std::filesystem::path(fb)) &&
            dest_key(as_fb) != dest_key(preferred) && !dest_taken(as_fb, used))
        {
            err << "dumpfloppy: extract collision: '" << preferred.string()
                << "' already exists; writing '" << as_fb.string() << "'\n";
            return as_fb;
        }
    }

    const std::string stem = preferred.filename().stem().string();
    const std::string ext = preferred.filename().extension().string();
    const std::string base =
        stem.empty() ? preferred.filename().string() : stem;
    /* `.dup` is a live-name collision, not a deleted directory slot. */
    std::filesystem::path as_dup = in_parent(base + ".dup" + ext);
    if (!dest_taken(as_dup, used))
    {
        err << "dumpfloppy: extract collision: '" << preferred.string()
            << "' already exists; writing '" << as_dup.string() << "'\n";
        return as_dup;
    }
    for (int n = 2; n < 10000; ++n)
    {
        as_dup = in_parent(base + ".dup." + std::to_string(n) + ext);
        if (!dest_taken(as_dup, used))
        {
            err << "dumpfloppy: extract collision: '" << preferred.string()
                << "' already exists; writing '" << as_dup.string() << "'\n";
            return as_dup;
        }
    }
    return {};
}

bool cbm_extract_matches(const cbm_file& file, const extract_options& opt)
{
    if (opt.patterns.empty())
    {
        return true;
    }
    const std::string host = cbm_host_filename(file);
    const char* kind = cbm_file_kind_name(file.kind);
    for (const std::string& pat : opt.patterns)
    {
        if (glob_match(pat, file.name) || glob_match(pat, host) ||
            glob_match(pat, kind))
        {
            return true;
        }
    }
    return false;
}

int extract_cbm_files(const analysis& a, const extract_options& opt, std::ostream& err)
{
    if (ensure_real_directory(opt.dest_dir, err) != 0)
    {
        return -1;
    }

    int written = 0;
    int matched = 0;
    bool failed = false;
    std::unordered_set<std::string> used_dests;
    for (const cbm_file& file : a.cbm.entries)
    {
        if (!cbm_extract_matches(file, opt))
        {
            continue;
        }
        ++matched;
        const std::string host = host_leaf(cbm_host_filename(file), err);
        if (host.empty())
        {
            continue;
        }
        const std::filesystem::path rel(host);
        const std::filesystem::path preferred = opt.dest_dir / rel;
        if (has_symlink_component(preferred))
        {
            err << "dumpfloppy: skip symlink path '" << preferred.string() << "'\n";
            continue;
        }
        const std::filesystem::path dest =
            choose_extract_dest(preferred, host, used_dests, err);
        if (dest.empty())
        {
            err << "dumpfloppy: extract collision: no unique name for '"
                << preferred.string() << "'\n";
            failed = true;
            continue;
        }
        const std::vector<uint8_t> bytes =
            read_cbm_file(cbm_sector_bytes(a.image.bytes, a.cbm), file);
        const int placed = write_exclusive_bytes(dest, bytes, err);
        if (placed < 0)
        {
            failed = true;
            continue;
        }
        if (placed == 0)
        {
            continue;
        }
        used_dests.insert(dest_key(dest));
        ++written;
    }
    if (!opt.patterns.empty() && matched == 0)
    {
        err << "dumpfloppy: no files matched extract pattern\n";
        return -1;
    }
    if (failed)
    {
        return -1;
    }
    return written;
}

bool amiga_extract_matches(const amiga_file& file, const extract_options& opt)
{
    if (file.is_dir)
    {
        return false;
    }
    if (opt.patterns.empty())
    {
        return true;
    }
    const std::string host = amiga_host_filename(file);
    for (const std::string& pat : opt.patterns)
    {
        if (glob_match(pat, file.path) || glob_match(pat, file.name) ||
            glob_match(pat, host))
        {
            return true;
        }
    }
    return false;
}

int extract_amiga_files(const analysis& a, const extract_options& opt, std::ostream& err)
{
    if (ensure_real_directory(opt.dest_dir, err) != 0)
    {
        return -1;
    }

    int written = 0;
    int matched = 0;
    bool failed = false;
    std::unordered_set<std::string> used_dests;
    for (const amiga_file& file : a.amiga.entries)
    {
        if (!amiga_extract_matches(file, opt))
        {
            continue;
        }
        ++matched;
        const std::string host = host_leaf(amiga_host_filename(file), err);
        if (host.empty())
        {
            continue;
        }
        const std::filesystem::path rel(host);
        const std::filesystem::path preferred = opt.dest_dir / rel;
        if (has_symlink_component(preferred))
        {
            err << "dumpfloppy: skip symlink path '" << preferred.string() << "'\n";
            continue;
        }
        const std::filesystem::path dest =
            choose_extract_dest(preferred, host, used_dests, err);
        if (dest.empty())
        {
            err << "dumpfloppy: extract collision: no unique name for '"
                << preferred.string() << "'\n";
            failed = true;
            continue;
        }
        const std::vector<uint8_t> bytes = read_amiga_file(
            amiga_volume_bytes(a.image.bytes, a.amiga), a.amiga, file);
        const int placed = write_exclusive_bytes(dest, bytes, err);
        if (placed < 0)
        {
            failed = true;
            continue;
        }
        if (placed == 0)
        {
            continue;
        }
        used_dests.insert(dest_key(dest));
        ++written;
    }
    if (!opt.patterns.empty() && matched == 0)
    {
        err << "dumpfloppy: no files matched extract pattern\n";
        return -1;
    }
    if (failed)
    {
        return -1;
    }
    return written;
}

bool trd_extract_matches(const trd_file& file, const extract_options& opt)
{
    if (opt.patterns.empty())
    {
        return true;
    }
    const std::string host = trd_host_filename(file);
    for (const std::string& pat : opt.patterns)
    {
        if (glob_match(pat, file.name) || glob_match(pat, host) ||
            glob_match(pat, file.type_name))
        {
            return true;
        }
    }
    return false;
}

int extract_trd_files(const analysis& a, const extract_options& opt, std::ostream& err)
{
    if (ensure_real_directory(opt.dest_dir, err) != 0)
    {
        return -1;
    }

    int written = 0;
    int matched = 0;
    bool failed = false;
    std::unordered_set<std::string> used_dests;
    for (const trd_file& file : a.trd.entries)
    {
        if (!trd_extract_matches(file, opt))
        {
            continue;
        }
        ++matched;
        const std::string host = host_leaf(trd_host_filename(file), err);
        if (host.empty())
        {
            continue;
        }
        const std::filesystem::path rel(host);
        const std::filesystem::path preferred = opt.dest_dir / rel;
        if (has_symlink_component(preferred))
        {
            err << "dumpfloppy: skip symlink path '" << preferred.string() << "'\n";
            continue;
        }
        const std::filesystem::path dest =
            choose_extract_dest(preferred, host, used_dests, err);
        if (dest.empty())
        {
            err << "dumpfloppy: extract collision: no unique name for '"
                << host << "'\n";
            continue;
        }
        const std::vector<uint8_t> bytes = read_trd_file(a.image.bytes, file);
        const int placed = write_exclusive_bytes(dest, bytes, err);
        if (placed < 0)
        {
            failed = true;
            continue;
        }
        if (placed == 0)
        {
            continue;
        }
        used_dests.insert(dest_key(dest));
        ++written;
    }
    if (!opt.patterns.empty() && matched == 0)
    {
        err << "dumpfloppy: no files matched extract pattern\n";
        return -1;
    }
    if (failed)
    {
        return -1;
    }
    return written;
}

bool apple_extract_matches(const apple_file& file, const extract_options& opt)
{
    if (opt.patterns.empty())
    {
        return true;
    }
    const std::string host = apple_host_filename(file);
    for (const std::string& pat : opt.patterns)
    {
        if (glob_match(pat, file.name) || glob_match(pat, host) ||
            glob_match(pat, file.type_name))
        {
            return true;
        }
    }
    return false;
}

int extract_apple_files(const analysis& a, const extract_options& opt,
                        std::ostream& err)
{
    if (ensure_real_directory(opt.dest_dir, err) != 0)
    {
        return -1;
    }

    int written = 0;
    int matched = 0;
    bool failed = false;
    std::unordered_set<std::string> used_dests;
    for (const apple_file& file : a.apple.entries)
    {
        if (file.storage == 0x0Du)
        {
            continue;
        }
        if (!apple_extract_matches(file, opt))
        {
            continue;
        }
        ++matched;
        const std::string host = host_leaf(apple_host_filename(file), err);
        if (host.empty())
        {
            continue;
        }
        const std::filesystem::path rel(host);
        const std::filesystem::path preferred = opt.dest_dir / rel;
        if (has_symlink_component(preferred))
        {
            err << "dumpfloppy: skip symlink path '" << preferred.string() << "'\n";
            continue;
        }
        const std::filesystem::path dest =
            choose_extract_dest(preferred, host, used_dests, err);
        if (dest.empty())
        {
            err << "dumpfloppy: extract collision: no unique name for '"
                << host << "'\n";
            continue;
        }
        const std::vector<uint8_t> bytes = read_apple_file(a.apple, file);
        const int placed = write_exclusive_bytes(dest, bytes, err);
        if (placed < 0)
        {
            failed = true;
            continue;
        }
        if (placed == 0)
        {
            continue;
        }
        used_dests.insert(dest_key(dest));
        ++written;
    }
    if (!opt.patterns.empty() && matched == 0)
    {
        err << "dumpfloppy: no files matched extract pattern\n";
        return -1;
    }
    if (failed)
    {
        return -1;
    }
    return written;
}

} /* namespace */

bool extract_matches(const dir_entry& e, const extract_options& opt)
{
    if (!is_payload_file(e))
    {
        return false;
    }
    if (opt.patterns.empty())
    {
        return true;
    }
    for (const std::string& pat : opt.patterns)
    {
        if (glob_match(pat, e.name_83) || glob_match(pat, e.path) ||
            (!e.lfn.empty() && glob_match(pat, e.lfn)))
        {
            return true;
        }
    }
    return false;
}

int extract_files(const analysis& a, const extract_options& opt, std::ostream& err)
{
    if (!opt.enabled)
    {
        return 0;
    }
    extract_options rooted = opt;
    rooted.dest_dir = resolve_user_dest(opt.dest_dir);
    if (a.cbm.present)
    {
        return extract_cbm_files(a, rooted, err);
    }
    if (a.amiga.present)
    {
        return extract_amiga_files(a, rooted, err);
    }
    if (a.trd.present)
    {
        return extract_trd_files(a, rooted, err);
    }
    if (a.apple.present)
    {
        return extract_apple_files(a, rooted, err);
    }
    if (a.cpm)
    {
        err << "dumpfloppy: CP/M directory listing does not extract file bytes\n";
        return -1;
    }
    if (a.foreign.present && a.flux.assembled_chs.empty())
    {
        err << "dumpfloppy: cannot extract " << a.foreign.format
            << " flux/nibble images in this version\n";
        return -1;
    }
    if (a.flux.format_name == "86BOX 86F")
    {
        err << "dumpfloppy: cannot extract .86f flux images; sector map needs "
               "HxC .mfm or an 86F decoder\n";
        return -1;
    }
    if (ensure_real_directory(rooted.dest_dir, err) != 0)
    {
        return -1;
    }

    int written = 0;
    int matched = 0;
    bool failed = false;
    std::unordered_set<std::string> used_dests;
    const sector_store store = make_sector_store(a);
    for (const dir_entry& e : a.entries)
    {
        if (!extract_matches(e, opt))
        {
            continue;
        }
        ++matched;
        const std::string host = host_fat_path(fat_host_string(e), err);
        if (host.empty())
        {
            continue;
        }
        const std::filesystem::path rel(host);
        const std::filesystem::path preferred = rooted.dest_dir / rel;
        /* operator/ discards dest_dir when rel is absolute. host_fat_path
         * already rejected those; keep the guard so a leaf cannot escape. */
        if (preferred.is_absolute() && rel.is_absolute())
        {
            err << "dumpfloppy: skip unsafe path '" << rel.string() << "'\n";
            continue;
        }
        const std::string fallback = flatten_separators(mask_nonprintable(e.name_83));
        if (has_symlink_component(preferred))
        {
            err << "dumpfloppy: skip symlink path '" << preferred.string() << "'\n";
            continue;
        }
        const std::filesystem::path dest =
            choose_extract_dest(preferred, fallback, used_dests, err);
        if (dest.empty())
        {
            err << "dumpfloppy: extract collision: no unique name for '"
                << preferred.string() << "'\n";
            failed = true;
            continue;
        }
        const std::vector<uint8_t> bytes =
            read_file_contents(store.bytes, a.bpb, e);
        const int placed = write_exclusive_bytes(dest, bytes, err);
        if (placed < 0)
        {
            failed = true;
            continue;
        }
        if (placed == 0)
        {
            continue;
        }
        used_dests.insert(dest_key(dest));
        ++written;
    }
    for (const unused_run& run : a.unused)
    {
        bool want = opt.patterns.empty();
        if (!want)
        {
            for (const std::string& pat : opt.patterns)
            {
                if (glob_match(pat, run.host_name) || glob_match(pat, run.guess))
                {
                    want = true;
                    break;
                }
            }
        }
        if (!want)
        {
            continue;
        }
        ++matched;
        const std::string host = host_leaf(run.host_name, err);
        if (host.empty())
        {
            continue;
        }
        const std::filesystem::path rel(host);
        const std::filesystem::path preferred = rooted.dest_dir / rel;
        if (has_symlink_component(preferred))
        {
            err << "dumpfloppy: skip symlink path '" << preferred.string() << "'\n";
            continue;
        }
        const std::filesystem::path dest =
            choose_extract_dest(preferred, host, used_dests, err);
        if (dest.empty())
        {
            err << "dumpfloppy: extract collision: no unique name for '"
                << preferred.string() << "'\n";
            failed = true;
            continue;
        }
        const int placed = write_exclusive_bytes(dest, run.payload, err);
        if (placed < 0)
        {
            failed = true;
            continue;
        }
        if (placed == 0)
        {
            continue;
        }
        used_dests.insert(dest_key(dest));
        ++written;
    }
    if (!rooted.patterns.empty() && matched == 0)
    {
        err << "dumpfloppy: no files matched extract pattern\n";
        return -1;
    }
    if (failed)
    {
        return -1;
    }
    return written;
}

} /* namespace dumpfloppy */
