/**
 * @file format.h
 * @brief Abstract retro disk-image / file format. Unknown payloads are "DATA".
 *
 * Concrete formats live under `include/dumpfloppy/formats/` as self-standing
 * headers (one class per format) with a wiki URL in the file comment.
 */
#ifndef DUMPFLOPPY_FORMAT_H
#define DUMPFLOPPY_FORMAT_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace dumpfloppy
{

/** @brief Maximum Type-column width (listing). Longer labels are clipped. */
inline constexpr std::size_t k_type_column_width = 16;

/** @brief Disk container vs file payload. */
enum class format_kind
{
    disk_image, /**< Whole floppy/disk image (ADF, FAT12 IMA, WOZ, …). */
    file        /**< File inside a volume (font, EXE, graphic, …). */
};

/**
 * @brief One recognised format.
 *
 * @c type() defaults to `"DATA"` (same idea as file(1) for an unknown blob).
 * Overrides must return an uppercase label that fits @ref k_type_column_width.
 */
class file_format
{
public:
    virtual ~file_format() = default;

    /**
     * @brief Listing Type string.
     * @return Uppercase label; default `"DATA"`.
     */
    [[nodiscard]] virtual std::string type() const
    {
        return "DATA";
    }

    /**
     * @brief Canonical documentation URL for this format.
     */
    [[nodiscard]] virtual std::string_view source_url() const
    {
        return {};
    }

    /**
     * @brief Disk image vs in-volume file.
     */
    [[nodiscard]] virtual format_kind kind() const
    {
        return format_kind::file;
    }

    /**
     * @brief True when @p data looks like this format.
     *
     * @param[in] data Image or file bytes (may be empty).
     */
    [[nodiscard]] virtual bool detect(std::span<const uint8_t> data) const
    {
        (void)data;
        return false;
    }
};

/**
 * @brief Clip a type label to @ref k_type_column_width.
 */
[[nodiscard]] inline std::string clip_type_label(std::string_view label)
{
    if (label.size() <= k_type_column_width)
    {
        return std::string(label);
    }
    return std::string(label.substr(0, k_type_column_width));
}

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_FORMAT_H */
