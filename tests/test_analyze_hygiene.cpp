/**
 * @file test_analyze_hygiene.cpp
 * @brief analyze.hpp must not include CBM/Amiga/FAT/MFM parser headers.
 */
#include "dumpfloppy/analyze.hpp"

#ifdef DUMPFLOPPY_CBM_HPP
#error "analyze.hpp must not include cbm.hpp"
#endif
#ifdef DUMPFLOPPY_AMIGA_HPP
#error "analyze.hpp must not include amiga.hpp"
#endif
#ifdef DUMPFLOPPY_FAT_HPP
#error "analyze.hpp must not include fat.hpp"
#endif
#ifdef DUMPFLOPPY_IBM_MFM_HPP
#error "analyze.hpp must not include ibm_mfm.hpp"
#endif
#ifdef DUMPFLOPPY_TRD_HPP
#error "analyze.hpp must not include trd.hpp"
#endif

#include <catch2/catch_test_macros.hpp>

TEST_CASE("analyze.hpp exposes fs views without parser headers", "[analyze][hygiene]")
{
    dumpfloppy::analysis a{};
    REQUIRE_FALSE(a.cbm.present);
    REQUIRE_FALSE(a.amiga.present);
    REQUIRE_FALSE(a.flux.present);
    REQUIRE_FALSE(a.trd.present);
    REQUIRE(a.fat.kind == dumpfloppy::fat_kind::unknown);
#ifndef DUMPFLOPPY_CBM_VIEW_HPP
    FAIL("cbm_view.hpp should be visible via analyze.hpp");
#endif
#ifndef DUMPFLOPPY_AMIGA_VIEW_HPP
    FAIL("amiga_view.hpp should be visible via analyze.hpp");
#endif
#ifndef DUMPFLOPPY_FAT_VIEW_HPP
    FAIL("fat_view.hpp should be visible via analyze.hpp");
#endif
#ifndef DUMPFLOPPY_FLUX_VIEW_HPP
    FAIL("flux_view.hpp should be visible via analyze.hpp");
#endif
#ifndef DUMPFLOPPY_TRD_VIEW_HPP
    FAIL("trd_view.hpp should be visible via analyze.hpp");
#endif
}
