/**
 * @file catalog.hpp
 * @brief Known-disk table keyed by whole-image XXH64.
 */
#ifndef DUMPFLOPPY_CATALOG_HPP
#define DUMPFLOPPY_CATALOG_HPP

#include <string>
#include <string_view>

namespace dumpfloppy
{

/** @brief Catalog hit for a unique dump (container hash). */
struct catalog_hit
{
    bool found = false;
    std::string title{};
    std::string protection{};
};

/**
 * @brief Look up a whole-file XXH64 (16 lowercase hex chars).
 */
[[nodiscard]] catalog_hit catalog_lookup(std::string_view xxh64_hex);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_CATALOG_HPP */
