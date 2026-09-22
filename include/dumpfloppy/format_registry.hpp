/**
 * @file format_registry.hpp
 * @brief Ordered list of format detectors (specific first, DATA last).
 */
#ifndef DUMPFLOPPY_FORMAT_REGISTRY_HPP
#define DUMPFLOPPY_FORMAT_REGISTRY_HPP

#include "dumpfloppy/format.h"

#include <span>
#include <string>
#include <vector>

namespace dumpfloppy
{

/**
 * @brief All compiled-in format objects (disk images and files).
 *
 * @warning Pointers are to process-lifetime statics; do not free them.
 */
[[nodiscard]] std::vector<const file_format*> all_formats();

/**
 * @brief First matching format, or nullptr (caller then uses `"DATA"`).
 *
 * @param[in] data Bytes to sniff.
 * @param[in] kind If not defaulted, only this kind is considered.
 */
[[nodiscard]] const file_format*
identify_format(std::span<const uint8_t> data, format_kind kind);

/**
 * @brief Type label for @p data (`"DATA"` when nothing matches).
 */
[[nodiscard]] std::string identify_type(std::span<const uint8_t> data,
                                        format_kind kind);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_FORMAT_REGISTRY_HPP */
