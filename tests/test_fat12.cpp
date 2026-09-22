/**
 * @file test_fat12.cpp
 * @brief FAT12 nibble codec and packed even/odd round-trip.
 */
#include "dumpfloppy/fat12_codec.h"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

TEST_CASE("FAT12 classic 23 61 45 decodes to 0x123 and 0x456", "[fat12]")
{
    const std::array<uint8_t, 3> fat{{0x23, 0x61, 0x45}};
    uint16_t v = 0xFFFFu;
    REQUIRE(fat12_entry_get(fat.data(), fat.size(), 0, &v) == 0);
    REQUIRE(v == 0x123u);
    REQUIRE(fat12_entry_get(fat.data(), fat.size(), 1, &v) == 0);
    REQUIRE(v == 0x456u);
}

TEST_CASE("FAT12 set even then odd does not clobber the neighbour", "[fat12]")
{
    std::array<uint8_t, 3> fat{{0, 0, 0}};
    REQUIRE(fat12_entry_set(fat.data(), fat.size(), 0, 0x123u) == 0);
    REQUIRE(fat12_entry_set(fat.data(), fat.size(), 1, 0x456u) == 0);
    REQUIRE(fat[0] == 0x23u);
    REQUIRE(fat[1] == 0x61u);
    REQUIRE(fat[2] == 0x45u);
}

TEST_CASE("FAT12 get rejects NULL and truncated tables", "[fat12]")
{
    uint8_t fat[3] = {0, 0, 0};
    uint16_t v = 0;
    REQUIRE(fat12_entry_get(nullptr, 3, 0, &v) == -1);
    REQUIRE(fat12_entry_get(fat, 3, 0, nullptr) == -1);
    REQUIRE(fat12_entry_get(fat, 3, 2, &v) == -1);
    REQUIRE(fat12_entry_set(nullptr, 3, 0, 1) == -1);
    REQUIRE(fat12_entry_set(fat, 1, 0, 1) == -1);
}

TEST_CASE("FAT12 EOF/bad/free predicates", "[fat12]")
{
    REQUIRE(fat12_is_free(0) != 0);
    REQUIRE(fat12_is_bad(0xFF7) != 0);
    REQUIRE(fat12_is_eof(0xFFF) != 0);
    REQUIRE(fat12_is_eof(0xFF8) != 0);
    REQUIRE(fat12_is_reserved(0xFF0) != 0);
    REQUIRE(fat12_is_reserved(0xFF7) == 0);
}
