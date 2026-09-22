/**
 * @file util.hpp
 * @brief Little-endian readers, DOS timestamps, and small string helpers.
 */
#ifndef DUMPFLOPPY_UTIL_HPP
#define DUMPFLOPPY_UTIL_HPP

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace dumpfloppy
{

/**
 * @brief Read an unsigned 16-bit little-endian value.
 *
 * @param[in] data Image (or sector) bytes.
 * @param[in] off  Byte offset.
 * @return 0 if @p off is out of range.
 */
[[nodiscard]] inline uint16_t read_le16(std::span<const uint8_t> data, size_t off)
{
    if (off + 2u > data.size())
    {
        return 0;
    }
    return static_cast<uint16_t>(data[off] |
                                 (static_cast<uint16_t>(data[off + 1u]) << 8));
}

/**
 * @brief Read an unsigned 32-bit little-endian value.
 *
 * @param[in] data Image bytes.
 * @param[in] off  Byte offset.
 * @return 0 if @p off is out of range.
 */
[[nodiscard]] inline uint32_t read_le32(std::span<const uint8_t> data, size_t off)
{
    if (off + 4u > data.size())
    {
        return 0;
    }
    return static_cast<uint32_t>(data[off]) |
           (static_cast<uint32_t>(data[off + 1u]) << 8) |
           (static_cast<uint32_t>(data[off + 2u]) << 16) |
           (static_cast<uint32_t>(data[off + 3u]) << 24);
}

/**
 * @brief True when @p off .. @p off+len-1 lies inside @p data.
 */
[[nodiscard]] inline bool in_range(std::span<const uint8_t> data, size_t off,
                                   size_t len)
{
    if (len == 0u)
    {
        return off <= data.size();
    }
    if (off >= data.size())
    {
        return false;
    }
    return len <= data.size() - off;
}

/**
 * @brief Copy @p n bytes as a string, stopping at NUL, stripping trailing spaces.
 */
[[nodiscard]] std::string ascii_field(std::span<const uint8_t> data, size_t off,
                                      size_t n);

/**
 * @brief True if every byte is printable ASCII or space.
 */
[[nodiscard]] bool is_printable_ascii(std::string_view s);

/**
 * @brief Format a DOS 4+ volume serial as `XXXX-XXXX` (high word first).
 */
[[nodiscard]] std::string format_volume_serial(uint32_t serial);

/**
 * @brief Format a DOS packed date (`Yyyyyyymmmmddddd`, year since 1980).
 */
[[nodiscard]] std::string format_dos_date(uint16_t dos_date);

/**
 * @brief Format a DOS packed time (`hhhhhmmm mmmsssss`, seconds/2).
 */
[[nodiscard]] std::string format_dos_time(uint16_t dos_time);

/**
 * @brief Lowercase ASCII copy of @p s (file-extension compare).
 */
[[nodiscard]] std::string ascii_lower(std::string_view s);

/**
 * @brief SHA-256 of @p data as 64 lowercase hex characters.
 */
[[nodiscard]] std::string sha256_hex(std::span<const uint8_t> data);

/**
 * @brief Printable runs of length >= 4 inside @p data (boot-sector strings).
 */
[[nodiscard]] std::vector<std::string> printable_runs(std::span<const uint8_t> data,
                                                      size_t min_len = 4);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_UTIL_HPP */
