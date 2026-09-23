/**
 * @file foreign.hpp
 * @brief Parsers for SPS IPF, Apple WOZ, Pasti STX, and Apple 2IMG.
 *
 * These containers are flux or prefixed nibble dumps. @ref analyse stores
 * the result in @c analysis::foreign and skips FAT when @a present is true.
 */
#ifndef DUMPFLOPPY_FOREIGN_HPP
#define DUMPFLOPPY_FOREIGN_HPP

#include "dumpfloppy/foreign_view.hpp"

#include <span>
#include <cstdint>

namespace dumpfloppy
{

/**
 * @brief Parse IPF, WOZ, STX, or 2IMG. First match wins.
 *
 * @param[in] data Whole image.
 */
[[nodiscard]] foreign_disk parse_foreign(std::span<const uint8_t> data);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_FOREIGN_HPP */
