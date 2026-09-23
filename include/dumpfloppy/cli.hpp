/**
 * @file cli.hpp
 * @brief Order-independent argument parser for dumpfloppy.
 */
#ifndef DUMPFLOPPY_CLI_HPP
#define DUMPFLOPPY_CLI_HPP

#include "dumpfloppy/extract.hpp"
#include "dumpfloppy/report.hpp"
#include "dumpfloppy/update.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace dumpfloppy
{

/** @brief Parsed command line. */
struct cli_options
{
    bool help = false;
    bool version = false;
    bool ok = true;
    std::string error{};
    report_options report{};
    std::filesystem::path output{}; /**< Empty → stdout. */
    bool has_output = false;
    extract_options extract{};
    update_options update{};
    std::vector<std::filesystem::path> inputs{};
};

/**
 * @brief Parse @p argv. Options and image paths may be interleaved.
 *
 * @param[in] argc Argument count.
 * @param[in] argv Argument vector (`argv[0]` is the program name).
 * @return Parsed options. @a ok is false and @a error is set on a bad flag
 *         or a missing option argument.
 */
[[nodiscard]] cli_options parse_cli(int argc, char** argv);

/**
 * @brief Expand directory operands to `*.img` / `*.ima` / `*.mfm` / `*.86f`
 *        / `*.d64` (non-recursive).
 *
 * Explicit file operands are kept as-is. Hidden names are skipped in dirs.
 *
 * @param[in]  inputs Operand paths (files and/or directories).
 * @param[out] err    Set when directories yielded no matching images.
 * @return Flattened image paths (each directory’s matches sorted by path).
 */
[[nodiscard]] std::vector<std::filesystem::path>
expand_inputs(const std::vector<std::filesystem::path>& inputs, std::string& err);

/**
 * @brief Usage text (also printed on no-args).
 */
[[nodiscard]] std::string usage_text();

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_CLI_HPP */
