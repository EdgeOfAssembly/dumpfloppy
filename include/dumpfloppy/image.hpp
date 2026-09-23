/**
 * @file image.hpp
 * @brief Load a floppy image (`.img` / `.ima` / `.mfm` / `.86f` / `.d64` /
 *        `.d71` / `.d81` / `.adf`) into memory.
 */
#ifndef DUMPFLOPPY_IMAGE_HPP
#define DUMPFLOPPY_IMAGE_HPP

#include "dumpfloppy/types.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace dumpfloppy
{

/** @brief In-memory raw image plus identity metadata. */
struct floppy_image
{
    std::filesystem::path path{};
    std::vector<uint8_t> bytes{};
    std::string sha256{};
    std::string xxh64{};
    container_kind container = container_kind::unknown_raw;
    geometry size_geometry{};
};

/** @brief Refuse images larger than this (not a floppy). */
inline constexpr uint64_t k_max_image_bytes = 16u * 1024u * 1024u;

/**
 * @brief Read an entire floppy image.
 *
 * @param[in] path File to load.
 * @return Image on success; error string on failure.
 */
[[nodiscard]] std::expected<floppy_image, std::string>
load_image(const std::filesystem::path& path);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_IMAGE_HPP */
