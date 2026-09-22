/**
 * @file boot.cpp
 * @brief Decide DOS non-system vs DOS system vs custom booter.
 */
#include "dumpfloppy/boot.hpp"
#include "dumpfloppy/util.hpp"

#include <algorithm>
#include <cctype>

namespace dumpfloppy
{
namespace
{

bool contains_ci(std::string_view hay, std::string_view needle)
{
    if (needle.size() > hay.size())
    {
        return false;
    }
    auto it = std::search(hay.begin(), hay.end(), needle.begin(), needle.end(),
                          [](char a, char b)
                          {
                              const char aa = (a >= 'a' && a <= 'z')
                                                  ? static_cast<char>(a - 'a' + 'A')
                                                  : a;
                              const char bb = (b >= 'a' && b <= 'z')
                                                  ? static_cast<char>(b - 'a' + 'A')
                                                  : b;
                              return aa == bb;
                          });
    return it != hay.end();
}

} /* namespace */

boot_info classify_boot(std::span<const uint8_t> boot, const bpb_info& bpb)
{
    boot_info info{};
    if (boot.size() >= 512u)
    {
        info.has_aa55 = (boot[510] == 0x55u && boot[511] == 0xAAu);
    }
    else if (boot.size() >= 2u)
    {
        info.has_aa55 = (boot[boot.size() - 2u] == 0x55u &&
                         boot[boot.size() - 1u] == 0xAAu);
    }

    if (boot.size() >= 3u)
    {
        info.has_jump = (boot[0] == 0xEBu || boot[0] == 0xE9u);
    }

    for (const std::string& s : printable_runs(boot, 4))
    {
        bool has_alpha = false;
        for (const char c : s)
        {
            if (std::isalpha(static_cast<unsigned char>(c)) != 0)
            {
                has_alpha = true;
                break;
            }
        }
        if (has_alpha)
        {
            info.strings.push_back(s);
        }
    }

    for (const std::string& s : info.strings)
    {
        if (contains_ci(s, "IO      SYS") || contains_ci(s, "IO.SYS"))
        {
            info.mentions_io_sys = true;
        }
        if (contains_ci(s, "MSDOS   SYS") || contains_ci(s, "MSDOS.SYS"))
        {
            info.mentions_msdos_sys = true;
        }
        if (contains_ci(s, "IBMBIO") || contains_ci(s, "IBMDOS"))
        {
            info.mentions_ibmbio = true;
        }
        if (contains_ci(s, "Non-System") || contains_ci(s, "Non-System disk") ||
            contains_ci(s, "non system"))
        {
            info.mentions_non_system = true;
        }
    }

    const bool dos_loader = info.mentions_io_sys || info.mentions_msdos_sys ||
                            info.mentions_ibmbio || info.mentions_non_system;

    if (!info.has_jump && !info.has_aa55)
    {
        info.kind = boot_class::not_bootable;
        info.kind_text = "not bootable (no JMP, no 55 AA)";
        info.is_booter = false;
        return info;
    }
    if (!info.has_jump)
    {
        info.kind = boot_class::not_bootable;
        info.kind_text = "not bootable (55 AA present, but no JMP at offset 0)";
        info.is_booter = false;
        return info;
    }
    if (!info.has_aa55)
    {
        /* Atari ST and some copy-protected booters omit 55 AA. */
        info.kind = boot_class::custom_booter;
        info.kind_text = "custom booter (JMP present, no 55 AA — BIOS may still load it)";
        info.is_booter = true;
        return info;
    }
    if (dos_loader && bpb.looks_valid)
    {
        info.kind = boot_class::dos_non_system;
        info.kind_text =
            "MS-DOS boot sector, non-system volume (refined after root listing)";
        info.is_booter = false;
        return info;
    }
    info.kind = boot_class::custom_booter;
    info.kind_text = "custom / game booter (JMP + 55 AA, not the MS-DOS loader)";
    info.is_booter = true;
    return info;
}

void refine_boot_with_root(boot_info& boot, bool has_io_sys, bool has_msdos_sys)
{
    if (boot.kind != boot_class::dos_non_system &&
        boot.kind != boot_class::dos_system)
    {
        return;
    }
    if (has_io_sys || has_msdos_sys)
    {
        boot.kind = boot_class::dos_system;
        boot.kind_text = "MS-DOS system disk (boot sector + IO.SYS/IBMBIO in root)";
        boot.is_booter = false;
    }
    else
    {
        boot.kind = boot_class::dos_non_system;
        boot.kind_text =
            "MS-DOS formatted data disk (boot sector present; no IO.SYS in root)";
        boot.is_booter = false;
    }
}

} /* namespace dumpfloppy */
