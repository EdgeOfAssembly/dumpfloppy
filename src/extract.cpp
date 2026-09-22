/**
 * @file extract.cpp
 * @brief Cluster-walk extract of 8.3 files, including deleted names.
 */
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/directory.hpp"
#include "dumpfloppy/util.hpp"

#include <fstream>
#include <span>
#include <system_error>

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

bool path_is_safe(const std::filesystem::path& rel)
{
    for (const auto& part : rel)
    {
        const std::string s = part.string();
        if (s == ".." || s == ".")
        {
            return false;
        }
    }
    return true;
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
        const std::filesystem::path dest = opt.dest_dir / rel;
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
        const std::span<const uint8_t> volume =
            !a.flux.assembled_chs.empty()
                ? std::span<const uint8_t>(a.flux.assembled_chs)
                : std::span<const uint8_t>(a.image.bytes);
        const std::vector<uint8_t> bytes =
            read_file_contents(volume, a.bpb, e);
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
