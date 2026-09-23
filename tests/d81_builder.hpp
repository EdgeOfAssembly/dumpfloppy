/**
 * @file d81_builder.hpp
 * @brief Tiny in-memory 1581 D81 images for Catch2 tests.
 */
#ifndef DUMPFLOPPY_TEST_D81_BUILDER_HPP
#define DUMPFLOPPY_TEST_D81_BUILDER_HPP

#include "d64_builder.hpp"
#include "dumpfloppy/cbm.hpp"

#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>

namespace dumpfloppy_test
{

inline constexpr std::size_t k_d81_size = dumpfloppy::k_d81_bytes;

inline void d81_header_init(uint8_t* hdr, std::string_view name, std::string_view id)
{
    std::memset(hdr, 0, dumpfloppy::k_d64_sector_bytes);
    hdr[0] = 40;
    hdr[1] = 3;
    hdr[2] = static_cast<uint8_t>('D');
    hdr[3] = 0;
    poke_petscii(hdr + 4, 16, name);
    hdr[20] = 0xA0;
    hdr[21] = 0xA0;
    poke_petscii(hdr + 22, 2, id);
    hdr[24] = 0xA0;
    hdr[25] = static_cast<uint8_t>('3');
    hdr[26] = static_cast<uint8_t>('D');
    hdr[27] = 0xA0;
    hdr[28] = 0xA0;
}

/** @brief 1581 BAM sector: DOS version plus ones-complement (D81.TXT 40/1). */
inline void d81_bam_init(uint8_t* bam, uint8_t next_t, uint8_t next_s, uint8_t dos,
                         uint8_t id0, uint8_t id1)
{
    std::memset(bam, 0, dumpfloppy::k_d64_sector_bytes);
    bam[0] = next_t;
    bam[1] = next_s;
    bam[2] = dos;
    bam[3] = static_cast<uint8_t>(~dos);
    bam[4] = id0;
    bam[5] = id1;
}

/** @brief Distinctive 300-byte PRG for a D81 fixture. */
inline std::vector<uint8_t> sample_d81_prg_bytes()
{
    std::vector<uint8_t> p(300);
    for (std::size_t i = 0; i < p.size(); ++i)
    {
        p[i] = static_cast<uint8_t>((i + 0x81u) & 0xFFu);
    }
    const char tag[] = "D81-HELLO";
    std::memcpy(p.data(), tag, sizeof(tag) - 1u);
    return p;
}

/**
 * @brief 819200-byte D81 with header 40/0, directory 40/3, one closed PRG.
 *
 * Disk name `TEST 1581`, ID `81`, DOS type `3D`. PRG `HELLO81` at 1/0.
 */
inline std::vector<uint8_t> make_sample_d81()
{
    std::vector<uint8_t> img(k_d81_size, 0);
    uint8_t* hdr = img.data() + dumpfloppy::cbm_offset(dumpfloppy::cbm_media::d81,
                                                       40, 0);
    d81_header_init(hdr, "TEST 1581", "81");
    uint8_t* bam0 = img.data() + dumpfloppy::cbm_offset(dumpfloppy::cbm_media::d81,
                                                       40, 1);
    d81_bam_init(bam0, 40, 2, static_cast<uint8_t>('D'), hdr[22], hdr[23]);
    uint8_t* bam1 = img.data() + dumpfloppy::cbm_offset(dumpfloppy::cbm_media::d81,
                                                       40, 2);
    d81_bam_init(bam1, 0, 0xFF, static_cast<uint8_t>('D'), hdr[22], hdr[23]);

    const std::vector<uint8_t> prg = sample_d81_prg_bytes();
    write_cbm_chain(img, dumpfloppy::cbm_media::d81, 1, 0, prg);

    uint8_t* dir = img.data() + dumpfloppy::cbm_offset(dumpfloppy::cbm_media::d81,
                                                      40, 3);
    dir[0] = 0;
    dir[1] = 0xFF;
    write_dirent(dir, 0x82, 1, 0, "HELLO81", 2);
    return img;
}

} /* namespace dumpfloppy_test */

#endif /* DUMPFLOPPY_TEST_D81_BUILDER_HPP */
