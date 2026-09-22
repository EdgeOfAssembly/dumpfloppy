/**
 * @file version.hpp
 * @brief dumpfloppy package version (CLI `-v` / `--version`).
 */
#ifndef DUMPFLOPPY_VERSION_HPP
#define DUMPFLOPPY_VERSION_HPP

namespace dumpfloppy
{

/** @brief Semantic version string for this release. */
inline constexpr const char* k_version = "0.4";

/** @brief Program name used in usage and version lines. */
inline constexpr const char* k_program = "dumpfloppy";

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_VERSION_HPP */
