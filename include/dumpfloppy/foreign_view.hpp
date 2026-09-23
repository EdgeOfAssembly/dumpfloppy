/**
 * @file foreign_view.hpp
 * @brief IPF / WOZ / STX / 2IMG result types for @ref analysis (no parser).
 */
#ifndef DUMPFLOPPY_FOREIGN_VIEW_HPP
#define DUMPFLOPPY_FOREIGN_VIEW_HPP

#include <cstdint>
#include <string>

namespace dumpfloppy
{

/** @brief Which non-FAT / non-CBM / non-ADF / non-TRD container matched. */
enum class foreign_kind : uint8_t
{
    none = 0,
    ipf = 1,     /**< SPS CAPS IPF. */
    woz = 2,     /**< Apple II WOZ1/WOZ2. */
    stx = 3,     /**< Atari ST Pasti STX. */
    img2mg = 4   /**< Apple 2IMG (.2mg). */
};

/**
 * @brief Parsed flux/nibble container; @a present skips FAT/BPB.
 */
struct foreign_disk
{
    bool present = false;
    foreign_kind kind = foreign_kind::none;
    std::string format{};     /**< `SPS IPF`, `APPLE WOZ`, … */
    std::string platform{};   /**< Amiga / Apple II / Atari ST. */
    std::string creator{};
    uint32_t min_cylinder = 0;
    uint32_t max_cylinder = 0;
    uint32_t min_head = 0;
    uint32_t max_head = 0;
    uint32_t track_count = 0; /**< IMGE chunks, or WOZ tracks used. */
    uint32_t file_id = 0;     /**< SPS file id when IPF. */
    uint32_t data_offset = 0; /**< 2IMG payload offset. */
    uint32_t data_length = 0; /**< 2IMG payload length. */
    bool write_protected = false;
    std::string note{};
};

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_FOREIGN_VIEW_HPP */
