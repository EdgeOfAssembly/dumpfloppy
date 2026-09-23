/**
 * @file extract.cpp
 * @brief Cluster-walk extract of 8.3 files, including deleted names.
 */
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/amiga.hpp"
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/cbm.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/trd.hpp"
#include "dumpfloppy/util.hpp"
#include "dumpfloppy/volume.hpp"

#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>

namespace dumpfloppy
{
namespace
{

std::filesystem::path host_relative(const dir_entry& e)
{
    std::string rel = e.path.empty() ? e.name_83 : e.path;
    for (char& c : rel)
    {
        if (c == '\\')
        {
            c = '/';
        }
    }
    return std::filesystem::path(rel);
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
    return std::filesystem::exists(p, ec);
}

/**
 * @brief Host path for @p preferred; on collision, @p fallback_name or
 *        `stem.deleted.ext` so a prior payload is not trunc-overwritten.
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
    std::filesystem::path as_del = in_parent(base + ".deleted" + ext);
    if (!dest_taken(as_del, used))
    {
        err << "dumpfloppy: extract collision: '" << preferred.string()
            << "' already exists; writing '" << as_del.string() << "'\n";
        return as_del;
    }
    for (int n = 2; n < 10000; ++n)
    {
        as_del = in_parent(base + ".deleted." + std::to_string(n) + ext);
        if (!dest_taken(as_del, used))
        {
            err << "dumpfloppy: extract collision: '" << preferred.string()
                << "' already exists; writing '" << as_del.string() << "'\n";
            return as_del;
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
    std::error_code ec{};
    std::filesystem::create_directories(opt.dest_dir, ec);
    if (ec)
    {
        err << "dumpfloppy: cannot create '" << opt.dest_dir.string()
            << "': " << ec.message() << '\n';
        return -1;
    }

    int written = 0;
    int matched = 0;
    std::unordered_set<std::string> used_dests;
    for (const cbm_file& file : a.cbm.entries)
    {
        if (!cbm_extract_matches(file, opt))
        {
            continue;
        }
        ++matched;
        const std::string host = cbm_host_filename(file);
        const std::filesystem::path rel(host);
        if (!path_is_safe(rel))
        {
            err << "dumpfloppy: skip unsafe path '" << rel.string() << "'\n";
            continue;
        }
        const std::filesystem::path preferred = opt.dest_dir / rel;
        const std::filesystem::path dest =
            choose_extract_dest(preferred, host, used_dests, err);
        if (dest.empty())
        {
            err << "dumpfloppy: extract collision: no unique name for '"
                << preferred.string() << "'\n";
            return -1;
        }
        if (dest.has_parent_path())
        {
            std::filesystem::create_directories(dest.parent_path(), ec);
            if (ec)
            {
                err << "dumpfloppy: cannot create '" << dest.parent_path().string()
                    << "': " << ec.message() << '\n';
                return -1;
            }
        }
        const std::vector<uint8_t> bytes =
            read_cbm_file(cbm_sector_bytes(a.image.bytes, a.cbm), file);
        std::ofstream out(dest, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            err << "dumpfloppy: cannot write '" << dest.string() << "'\n";
            return -1;
        }
        if (!bytes.empty())
        {
            out.write(reinterpret_cast<const char*>(bytes.data()),
                      static_cast<std::streamsize>(bytes.size()));
        }
        if (!out)
        {
            err << "dumpfloppy: short write '" << dest.string() << "'\n";
            return -1;
        }
        used_dests.insert(dest_key(dest));
        ++written;
    }
    if (!opt.patterns.empty() && matched == 0)
    {
        err << "dumpfloppy: no files matched extract pattern\n";
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
    std::error_code ec{};
    std::filesystem::create_directories(opt.dest_dir, ec);
    if (ec)
    {
        err << "dumpfloppy: cannot create '" << opt.dest_dir.string()
            << "': " << ec.message() << '\n';
        return -1;
    }

    int written = 0;
    int matched = 0;
    std::unordered_set<std::string> used_dests;
    for (const amiga_file& file : a.amiga.entries)
    {
        if (!amiga_extract_matches(file, opt))
        {
            continue;
        }
        ++matched;
        const std::string host = amiga_host_filename(file);
        const std::filesystem::path rel(host);
        if (!path_is_safe(rel))
        {
            err << "dumpfloppy: skip unsafe path '" << rel.string() << "'\n";
            continue;
        }
        const std::filesystem::path preferred = opt.dest_dir / rel;
        const std::filesystem::path dest =
            choose_extract_dest(preferred, host, used_dests, err);
        if (dest.empty())
        {
            err << "dumpfloppy: extract collision: no unique name for '"
                << preferred.string() << "'\n";
            return -1;
        }
        if (dest.has_parent_path())
        {
            std::filesystem::create_directories(dest.parent_path(), ec);
            if (ec)
            {
                err << "dumpfloppy: cannot create '" << dest.parent_path().string()
                    << "': " << ec.message() << '\n';
                return -1;
            }
        }
        const std::vector<uint8_t> bytes =
            read_amiga_file(a.image.bytes, a.amiga, file);
        std::ofstream out(dest, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            err << "dumpfloppy: cannot write '" << dest.string() << "'\n";
            return -1;
        }
        if (!bytes.empty())
        {
            out.write(reinterpret_cast<const char*>(bytes.data()),
                      static_cast<std::streamsize>(bytes.size()));
        }
        if (!out)
        {
            err << "dumpfloppy: short write '" << dest.string() << "'\n";
            return -1;
        }
        used_dests.insert(dest_key(dest));
        ++written;
    }
    if (!opt.patterns.empty() && matched == 0)
    {
        err << "dumpfloppy: no files matched extract pattern\n";
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
    std::error_code ec{};
    std::filesystem::create_directories(opt.dest_dir, ec);
    if (ec)
    {
        err << "dumpfloppy: cannot create '" << opt.dest_dir.string()
            << "': " << ec.message() << '\n';
        return -1;
    }

    int written = 0;
    int matched = 0;
    std::unordered_set<std::string> used_dests;
    for (const trd_file& file : a.trd.entries)
    {
        if (!trd_extract_matches(file, opt))
        {
            continue;
        }
        ++matched;
        const std::string host = trd_host_filename(file);
        const std::filesystem::path rel(host);
        if (!path_is_safe(rel))
        {
            err << "dumpfloppy: skip unsafe path '" << rel.string() << "'\n";
            continue;
        }
        const std::filesystem::path preferred = opt.dest_dir / rel;
        const std::filesystem::path dest =
            choose_extract_dest(preferred, host, used_dests, err);
        if (dest.empty())
        {
            err << "dumpfloppy: extract collision: no unique name for '"
                << host << "'\n";
            continue;
        }
        const std::vector<uint8_t> bytes = read_trd_file(a.image.bytes, file);
        std::ofstream out(dest, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            err << "dumpfloppy: cannot write '" << dest.string() << "'\n";
            return -1;
        }
        if (!bytes.empty())
        {
            out.write(reinterpret_cast<const char*>(bytes.data()),
                      static_cast<std::streamsize>(bytes.size()));
        }
        if (!out)
        {
            err << "dumpfloppy: short write '" << dest.string() << "'\n";
            return -1;
        }
        used_dests.insert(dest_key(dest));
        ++written;
    }
    if (!opt.patterns.empty() && matched == 0)
    {
        err << "dumpfloppy: no files matched extract pattern\n";
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
    if (a.cbm.present)
    {
        return extract_cbm_files(a, opt, err);
    }
    if (a.amiga.present)
    {
        return extract_amiga_files(a, opt, err);
    }
    if (a.trd.present)
    {
        return extract_trd_files(a, opt, err);
    }
    if (a.foreign.present)
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
    std::error_code ec{};
    std::filesystem::create_directories(opt.dest_dir, ec);
    if (ec)
    {
        err << "dumpfloppy: cannot create '" << opt.dest_dir.string()
            << "': " << ec.message() << '\n';
        return -1;
    }

    int written = 0;
    int matched = 0;
    std::unordered_set<std::string> used_dests;
    const sector_store store = make_sector_store(a);
    for (const dir_entry& e : a.entries)
    {
        if (!extract_matches(e, opt))
        {
            continue;
        }
        ++matched;
        const std::filesystem::path rel = host_relative(e);
        if (!path_is_safe(rel))
        {
            err << "dumpfloppy: skip unsafe path '" << rel.string() << "'\n";
            continue;
        }
        const std::filesystem::path preferred = opt.dest_dir / rel;
        if (preferred.is_absolute() && rel.is_absolute())
        {
            err << "dumpfloppy: skip unsafe path '" << rel.string() << "'\n";
            continue;
        }
        const std::filesystem::path dest =
            choose_extract_dest(preferred, e.name_83, used_dests, err);
        if (dest.empty())
        {
            err << "dumpfloppy: extract collision: no unique name for '"
                << preferred.string() << "'\n";
            return -1;
        }
        if (dest.has_parent_path())
        {
            std::filesystem::create_directories(dest.parent_path(), ec);
            if (ec)
            {
                err << "dumpfloppy: cannot create '" << dest.parent_path().string()
                    << "': " << ec.message() << '\n';
                return -1;
            }
        }
        const std::vector<uint8_t> bytes =
            read_file_contents(store.bytes, a.bpb, e);
        std::ofstream out(dest, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            err << "dumpfloppy: cannot write '" << dest.string() << "'\n";
            return -1;
        }
        if (!bytes.empty())
        {
            out.write(reinterpret_cast<const char*>(bytes.data()),
                      static_cast<std::streamsize>(bytes.size()));
        }
        if (!out)
        {
            err << "dumpfloppy: short write '" << dest.string() << "'\n";
            return -1;
        }
        used_dests.insert(dest_key(dest));
        ++written;
    }
    if (!opt.patterns.empty() && matched == 0)
    {
        err << "dumpfloppy: no files matched extract pattern\n";
        return -1;
    }
    return written;
}

} /* namespace dumpfloppy */
