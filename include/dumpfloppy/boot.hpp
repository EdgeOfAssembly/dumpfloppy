/**
 * @file boot.hpp
 * @brief Boot-sector classification (BIOS signature, DOS vs game booter).
 */
#ifndef DUMPFLOPPY_BOOT_HPP
#define DUMPFLOPPY_BOOT_HPP

#include "dumpfloppy/types.hpp"

#include <span>
#include <string>
#include <vector>

namespace dumpfloppy
{

/** @brief Boot-sector findings independent of the directory tree. */
struct boot_info
{
    bool has_aa55 = false;
    bool has_jump = false;
    boot_class kind = boot_class::not_bootable;
    std::string kind_text{};
    bool is_booter = false; /**< Game/custom payload boot, not DOS. */
    std::vector<std::string> strings{};
    bool mentions_io_sys = false;
    bool mentions_msdos_sys = false;
    bool mentions_ibmbio = false;
    bool mentions_non_system = false;
};

/**
 * @brief Classify the boot sector.
 *
 * @param[in] boot Boot sector bytes (typically 512).
 * @param[in] bpb  Parsed BPB (may be invalid).
 */
[[nodiscard]] boot_info classify_boot(std::span<const uint8_t> boot,
                                      const bpb_info& bpb);

/**
 * @brief Update @p boot after the root directory is known (system files).
 *
 * @param[in,out] boot           Classification to refine.
 * @param[in]     has_io_sys     Root contains IO.SYS or IBMBIO.COM (not deleted).
 * @param[in]     has_msdos_sys  Root contains MSDOS.SYS or IBMDOS.COM.
 */
void refine_boot_with_root(boot_info& boot, bool has_io_sys, bool has_msdos_sys);

} /* namespace dumpfloppy */

#endif /* DUMPFLOPPY_BOOT_HPP */
