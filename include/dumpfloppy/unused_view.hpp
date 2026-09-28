/**
 * @file unused_view.hpp
 * @brief Leftover data in FAT-free / FAT-bad clusters (no parser).
 *
 * Directory-deleted 8.3 names are @ref dir_entry::deleted. This view is
 * clusters the FAT marks free or bad that still hold non-fill bytes
 * (wiped source, slack, copy-protection marks).
 */
#ifndef DUMPFLOPPY_UNUSED_VIEW_HPP
#define DUMPFLOPPY_UNUSED_VIEW_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace dumpfloppy
{

/** @brief Why this run is not a live FAT chain. */
enum class unused_kind : uint8_t
{
    fat_free = 0, /**< FAT entry 0 (unallocated). */
    fat_bad = 1   /**< FAT bad marker (often leftover or protection). */
};

/**
 * @brief One contiguous run of leftover clusters.
 *
 * @a payload is the concatenated cluster bytes with trailing NULs on the
 * last cluster trimmed. @a host_name is a safe extract basename
 * (`unused_c0327.c`).
 */
struct unused_run
{
    uint16_t first_cluster = 0;
    uint16_t last_cluster = 0;
    unused_kind kind = unused_kind::fat_free;
    std::string guess{};     /**< `C source`, `text`, or `binary`. */
    std::string host_name{}; /**< Extract basename. */
    std::string xxh64{};     /**< Seed-0 XXH64 of @a payload. */
    std::string preview{};   /**< Short printable peek for the listing. */
    std::vector<uint8_t> payload{};
};

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_UNUSED_VIEW_HPP */
