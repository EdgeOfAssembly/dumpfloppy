/**
 * @file main.cpp
 * @brief dumpfloppy entry: load images, analyse, print secrets.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/cli.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/forensics.hpp"
#include "dumpfloppy/ibm_mfm.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/offset.hpp"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/update.hpp"
#include "dumpfloppy/version.hpp"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace
{

int write_one(const dumpfloppy::analysis& a, const dumpfloppy::report_options& opt,
              const std::filesystem::path& dest, bool to_file)
{
    if (!to_file)
    {
        dumpfloppy::write_report(a, std::cout, opt);
        return 0;
    }
    std::error_code ec{};
    const auto parent = dest.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent, ec);
        if (ec)
        {
            std::cerr << "dumpfloppy: cannot create '" << parent.string()
                      << "': " << ec.message() << '\n';
            return 1;
        }
    }
    std::ofstream out(dest, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        std::cerr << "dumpfloppy: cannot write '" << dest.string() << "'\n";
        return 1;
    }
    dumpfloppy::write_report(a, out, opt);
    return 0;
}

std::filesystem::path report_path_for(const std::filesystem::path& output,
                                      const std::filesystem::path& input,
                                      size_t input_count, bool output_is_dir)
{
    if (output_is_dir)
    {
        return output / (input.stem().string() + ".txt");
    }
    if (input_count == 1u)
    {
        return output;
    }
    /* Multi-input + file -o: outstem_instem.txt */
    const std::string stem = output.stem().string();
    const auto parent = output.parent_path();
    const std::filesystem::path name = stem + "_" + input.stem().string() + ".txt";
    return parent.empty() ? name : (parent / name);
}

void remove_quiet(const std::filesystem::path& path)
{
    std::error_code ec{};
    std::filesystem::remove(path, ec);
}

/**
 * @brief Replace @p dest with @p bytes without truncating @p dest first.
 *
 * Writes a sibling temp (`dest` + `.dumpfloppy-tmp`), flushes, closes, checks
 * the on-disk size, then `rename`s over @p dest. Any failure leaves the original
 * file intact and removes the temp if it was created.
 *
 * @param[in] dest  Image path to replace (same directory as the temp).
 * @param[in] bytes New file contents.
 * @return 0 on success, 1 after a diagnostic on stderr.
 */
int replace_file_atomic(const std::filesystem::path& dest,
                        const std::vector<uint8_t>& bytes)
{
    std::filesystem::path tmp = dest;
    tmp += ".dumpfloppy-tmp";
    remove_quiet(tmp);

    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc | std::ios::out);
        if (!out)
        {
            std::cerr << "dumpfloppy: cannot write '" << tmp.string() << "'\n";
            return 1;
        }
        if (!bytes.empty())
        {
            out.write(reinterpret_cast<const char*>(bytes.data()),
                      static_cast<std::streamsize>(bytes.size()));
        }
        out.flush();
        if (!out)
        {
            std::cerr << "dumpfloppy: short write '" << tmp.string() << "'\n";
            out.close();
            remove_quiet(tmp);
            return 1;
        }
        out.close();
        if (!out)
        {
            std::cerr << "dumpfloppy: cannot close '" << tmp.string() << "'\n";
            remove_quiet(tmp);
            return 1;
        }
    }

    std::error_code ec{};
    const auto sz = std::filesystem::file_size(tmp, ec);
    if (ec || sz != static_cast<std::uintmax_t>(bytes.size()))
    {
        std::cerr << "dumpfloppy: incomplete write '" << tmp.string() << "'\n";
        remove_quiet(tmp);
        return 1;
    }

    std::filesystem::rename(tmp, dest, ec);
    if (ec)
    {
        std::cerr << "dumpfloppy: cannot replace '" << dest.string()
                  << "': " << ec.message() << '\n';
        remove_quiet(tmp);
        return 1;
    }
    return 0;
}

/**
 * @brief Print the FAT offset map, or the non-FAT diagnostic.
 *
 * @param[in] a          Analysis of one image.
 * @param[in] has_offset True when `--offset` was passed.
 * @param[in] offset     Byte offset from `--offset`.
 * @return 0 when the line was printed or the flag is off, 1 when the
 *         image is not FAT12/FAT16.
 */
int write_offset_line(const dumpfloppy::analysis& a, bool has_offset, uint64_t offset)
{
    if (!has_offset)
    {
        return 0;
    }
    if (a.kind != dumpfloppy::fat_kind::fat12 && a.kind != dumpfloppy::fat_kind::fat16)
    {
        std::cerr << "dumpfloppy: offset map is only implemented for FAT12/FAT16\n";
        return 1;
    }
    std::cout << dumpfloppy::format_fat_offset(a, offset) << '\n';
    return 0;
}

} /* namespace */

int main(int argc, char** argv)
{
    dumpfloppy::cli_options cli = dumpfloppy::parse_cli(argc, argv);
    if (!cli.ok)
    {
        std::cerr << "dumpfloppy: " << cli.error << '\n';
        std::cerr << dumpfloppy::usage_text();
        return 1;
    }
    if (cli.help || (argc == 1))
    {
        std::cout << dumpfloppy::usage_text();
        return 0;
    }
    if (cli.version)
    {
        std::cout << dumpfloppy::k_program << " " << dumpfloppy::k_version << '\n';
        return 0;
    }

    const bool mutating = cli.update.enabled || cli.extract.enabled;
    const bool want_forensics = cli.forensics.slack || cli.forensics.leaked ||
                                cli.forensics.carve || cli.forensics.sources;
    if (cli.inputs.empty())
    {
        if (mutating || cli.has_offset || want_forensics)
        {
            std::cerr << "dumpfloppy: no image files given\n";
            return 1;
        }
        std::cout << dumpfloppy::usage_text();
        return 0;
    }

    std::string expand_err;
    const std::vector<std::filesystem::path> files =
        dumpfloppy::expand_inputs(cli.inputs, expand_err);
    if (files.empty())
    {
        std::cerr << "dumpfloppy: "
                  << (expand_err.empty() ? "no image files given" : expand_err)
                  << '\n';
        return 1;
    }

    if (cli.has_output && cli.extract.enabled)
    {
        std::error_code ec{};
        if (std::filesystem::is_regular_file(cli.output, ec))
        {
            std::cerr << "dumpfloppy: -o '" << cli.output.string()
                      << "' is an existing file, not an extract directory\n";
            return 1;
        }
        cli.extract.dest_dir = cli.output;
    }
    else if (cli.has_output && cli.update.enabled)
    {
        std::cerr << "dumpfloppy: warning: -o is ignored with -u (report-only)\n";
    }

    bool output_is_dir = false;
    if (cli.has_output && !mutating)
    {
        std::error_code ec{};
        if (std::filesystem::is_directory(cli.output, ec) ||
            cli.output.string().ends_with('/'))
        {
            output_is_dir = true;
            std::filesystem::create_directories(cli.output, ec);
            if (ec)
            {
                std::cerr << "dumpfloppy: cannot create '" << cli.output.string()
                          << "': " << ec.message() << '\n';
                return 1;
            }
        }
        else if (files.size() > 1u)
        {
            std::cerr << "dumpfloppy: warning: multiple inputs with -o file; "
                         "writing "
                      << cli.output.stem().string() << "_<stem>.txt per image\n";
        }
    }

    int rc = 0;
    auto emit_forensics = [&](const dumpfloppy::analysis& image)
    {
        if (!want_forensics)
        {
            return;
        }
        if (dumpfloppy::write_forensics(image, cli.forensics, std::cout, std::cerr) != 0)
        {
            rc = 1;
        }
    };
    for (size_t i = 0; i < files.size(); ++i)
    {
        auto loaded = dumpfloppy::load_image(files[i]);
        if (!loaded)
        {
            std::cerr << "dumpfloppy: " << loaded.error() << '\n';
            rc = 1;
            continue;
        }
        dumpfloppy::analysis a = dumpfloppy::analyse(std::move(*loaded));
        if (a.directory_capped)
        {
            std::cerr << "dumpfloppy: '" << files[i].string()
                      << "' directory walk hit cap:";
            if (a.directory_cap_depth)
            {
                std::cerr << " depth";
            }
            if (a.directory_cap_entries)
            {
                std::cerr << " entries";
            }
            if (a.directory_cap_chain)
            {
                std::cerr << " chain";
            }
            std::cerr << '\n';
        }
        if (cli.update.enabled)
        {
            if (dumpfloppy::update_files(a, cli.update, std::cerr) < 0)
            {
                rc = 1;
                continue;
            }
            std::vector<uint8_t> out_bytes;
            if (a.flux.present && a.flux.format_name == "HXC MFM")
            {
                std::vector<uint8_t> mfm = a.image.bytes;
                if (!dumpfloppy::patch_mfm_chs(mfm, a.flux, a.flux.assembled_chs))
                {
                    std::cerr << "dumpfloppy: MFM encode failed for '"
                              << files[i].string() << "'\n";
                    rc = 1;
                    continue;
                }
                out_bytes = std::move(mfm);
            }
            else
            {
                out_bytes = a.image.bytes;
            }
            if (replace_file_atomic(files[i], out_bytes) != 0)
            {
                rc = 1;
                continue;
            }
            if (write_offset_line(a, cli.has_offset, cli.offset) != 0)
            {
                rc = 1;
            }
            if (cli.extract.enabled)
            {
                if (dumpfloppy::extract_files(a, cli.extract, std::cerr) < 0)
                {
                    rc = 1;
                }
            }
            emit_forensics(a);
            continue;
        }
        if (write_offset_line(a, cli.has_offset, cli.offset) != 0)
        {
            rc = 1;
        }
        if (cli.extract.enabled)
        {
            if (dumpfloppy::extract_files(a, cli.extract, std::cerr) < 0)
            {
                rc = 1;
            }
            emit_forensics(a);
            continue;
        }
        if (cli.has_offset)
        {
            emit_forensics(a);
            continue;
        }
        if (cli.has_output)
        {
            const std::filesystem::path dest =
                report_path_for(cli.output, files[i], files.size(), output_is_dir);
            if (write_one(a, cli.report, dest, true) != 0)
            {
                rc = 1;
            }
        }
        else
        {
            if (i != 0u)
            {
                std::cout << "\n";
            }
            dumpfloppy::write_report(a, std::cout, cli.report);
        }
        emit_forensics(a);
    }
    return rc;
}
