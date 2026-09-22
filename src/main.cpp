/**
 * @file main.cpp
 * @brief dumpfloppy entry: load images, analyse, print secrets.
 */
#include "dumpfloppy/analyze.hpp"
#include "dumpfloppy/cli.hpp"
#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/ibm_mfm.hpp"
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/update.hpp"
#include "dumpfloppy/version.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
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
    if (cli.inputs.empty())
    {
        std::cout << dumpfloppy::usage_text();
        return 0;
    }

    std::string expand_err;
    const std::vector<std::filesystem::path> files =
        dumpfloppy::expand_inputs(cli.inputs, expand_err);
    if (files.empty())
    {
        std::cerr << "dumpfloppy: " << expand_err << '\n';
        return 1;
    }

    bool output_is_dir = false;
    if (cli.has_output)
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
            std::ofstream img_out(files[i], std::ios::binary | std::ios::trunc);
            if (!img_out)
            {
                std::cerr << "dumpfloppy: cannot write '" << files[i].string() << "'\n";
                rc = 1;
                continue;
            }
            if (!out_bytes.empty())
            {
                img_out.write(reinterpret_cast<const char*>(out_bytes.data()),
                              static_cast<std::streamsize>(out_bytes.size()));
            }
            if (!img_out)
            {
                std::cerr << "dumpfloppy: short write '" << files[i].string() << "'\n";
                rc = 1;
                continue;
            }
            if (cli.extract.enabled)
            {
                if (dumpfloppy::extract_files(a, cli.extract, std::cerr) < 0)
                {
                    rc = 1;
                }
            }
            continue;
        }
        if (cli.extract.enabled)
        {
            if (dumpfloppy::extract_files(a, cli.extract, std::cerr) < 0)
            {
                rc = 1;
            }
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
    }
    return rc;
}
