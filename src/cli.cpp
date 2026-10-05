/**
 * @file cli.cpp
 * @brief CLI: no-args = help, `-h`/`-v`, interleaved operands, dir batch.
 */
#include "dumpfloppy/cli.hpp"
#include "dumpfloppy/util.hpp"
#include "dumpfloppy/version.hpp"

#include <algorithm>
#include <charconv>
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
           ext == ".g64" || ext == ".g71" || ext == ".trd" || ext == ".ipf" ||
           ext == ".woz" || ext == ".stx" || ext == ".2mg" || ext == ".dsk" ||
           ext == ".po";
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

/**
 * @brief Parse a decimal or `0x` hex byte offset.
 *
 * @param[in]  text  Token after `--offset` or the text after `=`.
 * @param[out] value Parsed magnitude on success.
 * @param[out] err   Diagnostic without the `dumpfloppy:` prefix.
 */
bool parse_offset_value(const std::string& text, uint64_t& value, std::string& err)
{
    value = 0;
    if (text.empty() || text[0] == '+' || text[0] == '-')
    {
        err = "invalid offset '" + text + "'";
        return false;
    }
    int base = 10;
    const char* begin = text.data();
    const char* end = begin + text.size();
    if (text.size() >= 2u && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
    {
        base = 16;
        begin += 2;
        if (begin == end)
        {
            err = "invalid offset '" + text + "'";
            return false;
        }
    }
    uint64_t parsed = 0;
    const std::from_chars_result got = std::from_chars(begin, end, parsed, base);
    if (got.ec != std::errc{} || got.ptr != end)
    {
        err = "invalid offset '" + text + "'";
        return false;
    }
    value = parsed;
    return true;
}

} /* namespace */

std::string usage_text()
{
    std::ostringstream os;
    os << "Usage: " << k_program << " [options] [images…]\n"
       << "\n"
       << "  images    Floppy images (.img / .ima / .mfm / .86f / .d64 / .d71 /\n"
       << "            .d81 / .adf / .g64 / .g71 / .trd / .ipf / .woz / .stx /\n"
       << "            .2mg / .dsk / .po) and/or directories.\n"
       << "            Directories expand to those extensions (batch; no --batch).\n"
       << "            Options and inputs may be interleaved.\n"
       << "\n"
       << "Dump BIOS boot sector, FAT12/16 BPB, volume serial and label,\n"
       << "directory (including deleted entries), Commodore D64/D71/D81/G64/G71 CBMFS,\n"
       << "Amiga OFS/FFS ADF, ZX Spectrum TR-DOS TRD, SPS IPF (standard\n"
       << "AmigaDOS tracks) / Apple WOZ (5.25 6-and-2 catalog) / Atari STX /\n"
       << "Apple 2IMG containers,\n"
       << "HxC/86F flux metadata,\n"
       << "copy-protection schemes,\n"
       << "and a whole-image XXH64 catalog lookup.\n"
       << "\n"
       << "Options:\n"
       << "  -h, --help           Show this help and exit\n"
       << "  -v, --version        Show version and exit\n"
       << "  -o, --output PATH    Write the report to a file or directory (default: stdout).\n"
       << "                       With -x, PATH is the extract directory (default: .).\n"
       << "                       An existing regular file is an error. Ignored with -u\n"
       << "                       unless -x is also set.\n"
       << "      --no-color       Disable ANSI colour (default: on)\n"
       << "      --no-hex         Skip boot-sector hex dump (default: dump)\n"
       << "      --no-deleted     Hide deleted directory entries in the listing only\n"
       << "                       (extract still includes them; default: show)\n"
       << "      --no-unused      Hide leftover FAT-free/bad cluster runs in the listing\n"
       << "                       (extract still writes them; default: show)\n"
       << "  -x, --extract [GLOB] Extract files (no listing). Default directory is `.`;\n"
       << "                       -o DIR selects it and is created if missing.\n"
       << "                       Default: all payloads, deleted and unused included.\n"
       << "                       Quote globs: -x '*.PKD' -x '5??.PKD'\n"
       << "  -u, --update FILE    Overwrite the same-named file in the image.\n"
       << "                       Repeatable. Silent (no listing). FAT: same size in-place;\n"
       << "                       grow/shrink allocates or frees clusters and relocates\n"
       << "                       later live files when sequential growth needs them.\n"
       << "                       D64/D71/D81 CBMFS and ADF: same-size in-place only.\n"
       << "                       Accepts -u FILE, -uFILE, and --update=FILE.\n"
       << "                       G64/G71 GCR, REL, TRD, and .86f cannot be updated in this version.\n"
       << "      --offset=N       FAT12/FAT16: map byte offset N (decimal or 0x hex) to\n"
       << "                       a sector, a cluster or reserved/FAT/root, and the owning\n"
       << "                       file, free, slack, or past-end. One line on stdout.\n"
       << "      --slack          FAT12/FAT16: unused tail of each live file (default: off).\n"
       << "      --leaked         FAT12/FAT16: slot-aligned directory entries in slack,\n"
       << "                       free clusters, and bytes past the filesystem (default: off).\n"
       << "      --carve          FAT12/FAT16: carve MZ/ZM, GIF, and long ASCII from slack,\n"
       << "                       free clusters, and bytes past the filesystem (default: off).\n"
       << "                       On a non-FAT image the listing is still printed, then\n"
       << "                       stderr says the flag is only implemented for FAT12/FAT16\n"
       << "                       (same for --slack and --leaked) and the exit status is 1.\n"
       << "                       A directory walk cap prints\n"
       << "                       dumpfloppy: '<image>' directory walk hit cap: depth,\n"
       << "                       entries, and/or chain. Depth 32 skips that subdirectory\n"
       << "                       only. The entries cap (4096) stops a subdirectory but\n"
       << "                       still lists later root slots. A directory chain stops at\n"
       << "                       8192 clusters. A file chain is not cut at 8192. Exit\n"
       << "                       status stays 0 when only a cap fired.\n"
       << "      --sources        FAT12/FAT16: source patterns in slack, free clusters,\n"
       << "                       and bytes past the filesystem (default: off).\n"
       << "                       Case-sensitive: #include, proc near, org 100h, uses crt,\n"
       << "                       and a BASIC line (one to five digits, a space, then a\n"
       << "                       letter) at a region start or after CR/LF.\n"
       << "                       === Source === is offset, kind, and text. Kinds:\n"
       << "                       include, proc-near, org-100h, uses-crt, basic.\n"
       << "                       At most 64 hits. The next hit stops the scan and writes\n"
       << "                       Warning: source scan stopped at cap to stderr.\n"
       << "                       Exit status stays 0 for that cap. On a non-FAT image the\n"
       << "                       listing is still printed, stderr says sources is only\n"
       << "                       implemented for FAT12/FAT16, and the status is 1.\n"
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
        if (!end_opts && arg == "--no-unused")
        {
            o.report.show_unused = false;
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
        if (!end_opts && (arg == "--offset" || arg.starts_with("--offset=")))
        {
            std::string text;
            if (arg == "--offset")
            {
                if (i + 1 >= argc || argv[i + 1] == nullptr)
                {
                    o.ok = false;
                    o.error = "missing N after --offset";
                    return o;
                }
                ++i;
                text = argv[i];
            }
            else
            {
                text = arg.substr(9);
            }
            uint64_t value = 0;
            if (!parse_offset_value(text, value, o.error))
            {
                o.ok = false;
                return o;
            }
            o.has_offset = true;
            o.offset = value;
            continue;
        }
        if (!end_opts && arg == "--slack")
        {
            o.forensics.slack = true;
            continue;
        }
        if (!end_opts && arg == "--leaked")
        {
            o.forensics.leaked = true;
            continue;
        }
        if (!end_opts && arg == "--carve")
        {
            o.forensics.carve = true;
            continue;
        }
        if (!end_opts && arg == "--sources")
        {
            o.forensics.sources = true;
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
        err = "no .img/.ima/.mfm/.86f/.d64/.d71/.d81/.adf/.g64/.g71/.trd/.ipf/.woz/.stx/.2mg/.dsk/.po files found in the given directories";
    }
    return out;
}

} /* namespace dumpfloppy */
