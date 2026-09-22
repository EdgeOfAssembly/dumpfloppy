/**
 * @file identify.hpp
 * @brief libmagic MIME type of a recovered payload.
 */
#ifndef DUMPFLOPPY_IDENTIFY_HPP
#define DUMPFLOPPY_IDENTIFY_HPP

#include <cstdint>
#include <span>
#include <string>

namespace dumpfloppy
{

/**
 * @brief MIME type of @p data via libmagic (`MAGIC_MIME_TYPE`).
 *
 * @param[in] data File bytes (may be empty).
 * @return A type such as `text/plain` or `application/octet-stream`.
 */
[[nodiscard]] std::string mime_type(std::span<const uint8_t> data);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_IDENTIFY_HPP */
