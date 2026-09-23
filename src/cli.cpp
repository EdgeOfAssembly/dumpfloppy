/**
 * @file cli.cpp
 * @brief CLI: no-args = help, `-h`/`-v`, interleaved operands, dir batch.
 */
#include "dumpfloppy/cli.hpp"
#include "dumpfloppy/util.hpp"
#include "dumpfloppy/version.hpp"

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <system_error>

namespace dumpfloppy
{
namespace
{

bool is_floppy_ext(const std::filesystem::path& p)
{
    const std::string ext = ascii_lower(p.extension().string());
    return ext == ".img" || ext == ".ima" || ext == ".mfm" || ext == ".86f" ||
           ext == ".d64" || ext == ".d71" || ext == ".d81" || ext == ".adf" ||
           ext == ".g64";
}

bool looks_like_image_operand(const std::string& tok)
{
    if (tok.empty())
    {
        return false;
    }
    std::error_code ec{};
    const std::filesystem::path p(tok);
    if (std::filesystem::is_directory(p, ec))
    {
        return true;
    }
    return is_floppy_ext(p);
}

bool is_option_token(const std::string& tok)
{
    return tok.size() >= 2u && tok[0] == '-' && tok != "--";
}

} /* namespace */

std::string usage_text()
{
    std::ostringstream os;
    os << "Usage: " << k_program << " [options] [images…]\n"
       << "\n"
       << "  images    Floppy images (.img / .ima / .mfm / .86f / .d64 / .d71 /\n"
       << "            .d81 / .adf / .g64) and/or directories.\n"
       << "            Directories expand to those extensions (batch; no --batch).\n"
       << "            Options and inputs may be interleaved.\n"
       << "\n"
       << "Dump BIOS boot sector, FAT12/16 BPB, volume serial and label,\n"
       << "directory (including deleted entries), Commodore D64/D71/D81/G64 CBMFS,\n"
       << "Amiga OFS/FFS ADF, HxC/86F flux metadata, copy-protection schemes,\n"
       << "and a whole-image XXH64 catalog lookup.\n"
       << "\n"
       << "Options:\n"
       << "  -h, --help           Show this help and exit\n"
       << "  -v, --version        Show version and exit\n"
       << "  -o, --output PATH    Write report to a file or directory (default: stdout)\n"
       << "      --no-color       Disable ANSI colour (default: on)\n"
       << "      --no-hex         Skip boot-sector hex dump (default: dump)\n"
       << "      --no-deleted     Hide deleted directory entries in the listing only\n"
       << "                       (extract still includes them; default: show)\n"
       << "  -x, --extract [GLOB] Extract files to the current directory (no listing).\n"
       << "                       Default: all payloads, deleted included.\n"
       << "                       Quote globs: -x '*.PKD' -x '5??.PKD'\n"
       << "  -u, --update FILE    Overwrite the same-named file in the image.\n"
       << "                       Repeatable. Silent (no listing). FAT: same size in-place;\n"
       << "                       grow/shrink allocates or frees clusters and relocates\n"
       << "                       later live files when sequential growth needs them.\n"
       << "                       D64/D71/D81 CBMFS and ADF: same-size in-place only.\n"
       << "                       Accepts -u FILE, -uFILE, and --update=FILE.\n"
       << "                       G64 GCR, REL, and .86f cannot be updated in this version.\n"
       << "\n"
       << k_program << " " << k_version << "\n";
    return os.str();
}

cli_options parse_cli(int argc, char** argv)
{
    cli_options o{};
    bool end_opts = false;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i] != nullptr ? argv[i] : "";
        if (!end_opts && arg == "--")
        {
            end_opts = true;
            continue;
        }
        if (!end_opts && (arg == "-h" || arg == "--help"))
        {
            o.help = true;
            continue;
        }
        if (!end_opts && (arg == "-v" || arg == "--version"))
        {
            o.version = true;
            continue;
        }
        if (!end_opts && arg == "--no-color")
        {
            o.report.color = false;
            continue;
        }
        if (!end_opts && arg == "--no-hex")
        {
            o.report.hex_boot = false;
            continue;
        }
        if (!end_opts && arg == "--no-deleted")
        {
            o.report.show_deleted = false;
            continue;
        }
        if (!end_opts && (arg == "-x" || arg == "--extract"))
        {
            o.extract.enabled = true;
            if (i + 1 < argc && argv[i + 1] != nullptr)
            {
                const std::string next = argv[i + 1];
                const bool globby =
                    next.find('*') != std::string::npos ||
                    next.find('?') != std::string::npos;
                if (!is_option_token(next) &&
                    (globby || !looks_like_image_operand(next)))
                {
                    ++i;
                    o.extract.patterns.push_back(next);
                }
            }
            continue;
        }
        if (!end_opts && arg.starts_with("--extract="))
        {
            o.extract.enabled = true;
            o.extract.patterns.push_back(arg.substr(10));
            continue;
        }
        if (!end_opts && arg.starts_with("-x") && arg.size() > 2u)
        {
            o.extract.enabled = true;
            o.extract.patterns.push_back(arg.substr(2));
            continue;
        }
        if (!end_opts && (arg == "-u" || arg == "--update"))
        {
            if (i + 1 >= argc || argv[i + 1] == nullptr)
            {
                o.ok = false;
                o.error = "missing FILE after " + arg;
                return o;
            }
            ++i;
            o.update.enabled = true;
            o.update.hosts.emplace_back(argv[i]);
            continue;
        }
        if (!end_opts && arg.starts_with("--update="))
        {
            o.update.enabled = true;
            o.update.hosts.emplace_back(arg.substr(9));
            continue;
        }
        if (!end_opts && arg.starts_with("-u") && arg.size() > 2u)
        {
            o.update.enabled = true;
            o.update.hosts.emplace_back(arg.substr(2));
            continue;
        }
        if (!end_opts && (arg == "-o" || arg == "--output"))
        {
            if (i + 1 >= argc || argv[i + 1] == nullptr)
            {
                o.ok = false;
                o.error = "missing PATH after " + arg;
                return o;
            }
            ++i;
            o.output = argv[i];
            o.has_output = true;
            continue;
        }
        if (!end_opts && arg.starts_with("--output="))
        {
            o.output = arg.substr(9);
            o.has_output = true;
            continue;
        }
        if (!end_opts && arg.starts_with("-o") && arg.size() > 2u && arg != "-o")
        {
            o.output = arg.substr(2);
            o.has_output = true;
            continue;
        }
        if (!end_opts && !arg.empty() && arg[0] == '-')
        {
            o.ok = false;
            o.error = "unknown option: " + arg;
            return o;
        }
        o.inputs.emplace_back(arg);
    }
    return o;
}

std::vector<std::filesystem::path>
expand_inputs(const std::vector<std::filesystem::path>& inputs, std::string& err)
{
    std::vector<std::filesystem::path> out;
    err.clear();
    for (const std::filesystem::path& p : inputs)
    {
        std::error_code ec{};
        if (std::filesystem::is_directory(p, ec))
        {
            std::vector<std::filesystem::path> kids;
            for (const auto& ent : std::filesystem::directory_iterator(p, ec))
            {
                if (ec)
                {
                    break;
                }
                const auto name = ent.path().filename().string();
                if (name.empty() || name[0] == '.')
                {
                    continue;
                }
                if (ent.is_regular_file(ec) && is_floppy_ext(ent.path()))
                {
                    kids.push_back(ent.path());
                }
            }
            std::sort(kids.begin(), kids.end());
            out.insert(out.end(), kids.begin(), kids.end());
            continue;
        }
        out.push_back(p);
    }
    if (out.empty() && !inputs.empty())
    {
        err = "no .img/.ima/.mfm/.86f/.d64/.d71/.d81/.adf/.g64 files found in the given directories";
    }
    return out;
}

} /* namespace dumpfloppy */
