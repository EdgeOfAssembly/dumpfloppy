/**
 * @file format_registry.hpp
 * @brief Ordered list of format detectors (specific first, DATA last).
 */
#ifndef DUMPFLOPPY_FORMAT_REGISTRY_HPP
#define DUMPFLOPPY_FORMAT_REGISTRY_HPP

#include "dumpfloppy/format.h"

#include <span>
#include <string>
#include <string_view>
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
 * @brief First matching format of @p kind, or nullptr (caller then uses `"DATA"`).
 *
 * @param[in] data Bytes to sniff.
 * @param[in] kind Only this kind is considered (no default).
 */
[[nodiscard]] const file_format*
identify_format(std::span<const uint8_t> data, format_kind kind);

/**
 * @brief Type label: DATA, then extension (`match_name`), then magic.
 *
 * Name matching is first-wins (same as magic). Magic still overrides a
 * name hit, so a renamed `.COM` that starts `MZ` becomes EXE.
 */
[[nodiscard]] std::string identify_type(std::span<const uint8_t> data,
                                        format_kind kind,
                                        std::string_view name = {});

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_FORMAT_REGISTRY_HPP */
