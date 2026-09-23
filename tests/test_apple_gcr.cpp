/**
 * @file test_apple_gcr.cpp
 * @brief Apple II 6-and-2 / 4-and-4 codec round-trip.
 */
#include "dumpfloppy/apple_gcr_codec.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstring>
#include <vector>

TEST_CASE("Apple 6-and-2 nibble encode/decode round-trip", "[apple][gcr]")
{
    for (unsigned n = 0; n < 64u; ++n)
    {
        const uint8_t disk = apple_gcr_encode_6n2(static_cast<uint8_t>(n));
        REQUIRE((disk & 0x80u) != 0u);
        REQUIRE(apple_gcr_decode_6n2(disk) == static_cast<uint8_t>(n));
    }
}

TEST_CASE("Apple 6-and-2 unused bytes are invalid", "[apple][gcr]")
{
    REQUIRE(apple_gcr_decode_6n2(0x00) == APPLE_GCR_INVALID);
    REQUIRE(apple_gcr_decode_6n2(0xD5) == APPLE_GCR_INVALID);
    REQUIRE(apple_gcr_decode_6n2(0xAA) == APPLE_GCR_INVALID);
}

TEST_CASE("Apple 6-and-2 textbook codes", "[apple][gcr]")
{
    REQUIRE(apple_gcr_encode_6n2(0x00) == 0x96);
    REQUIRE(apple_gcr_encode_6n2(0x3F) == 0xFF);
    REQUIRE(apple_gcr_encode_6n2(0x0B) == 0xAD);
}

TEST_CASE("Apple 4-and-4 round-trip and NULL out", "[apple][gcr]")
{
    for (unsigned b = 0; b < 256u; ++b)
    {
        uint8_t odd = 0;
        uint8_t even = 0;
        uint8_t out = 0xCC;
        apple_gcr_encode_4n4(static_cast<uint8_t>(b), &odd, &even);
        REQUIRE(apple_gcr_decode_4n4(odd, even, &out) == 0);
        REQUIRE(out == static_cast<uint8_t>(b));
        REQUIRE((odd & 0xAAu) == 0xAAu);
        REQUIRE((even & 0xAAu) == 0xAAu);
    }
    uint8_t odd = 0;
    uint8_t even = 0;
    apple_gcr_encode_4n4(0x00, &odd, &even);
    REQUIRE(odd == 0xAA);
    REQUIRE(even == 0xAA);
    REQUIRE(apple_gcr_decode_4n4(odd, even, nullptr) == -1);
}

TEST_CASE("Apple 6-and-2 sector round-trip", "[apple][gcr]")
{
    uint8_t src[APPLE_GCR_SECTOR_BYTES] = {};
    uint8_t nib[APPLE_GCR_NIBBLE_BYTES] = {};
    uint8_t dst[APPLE_GCR_SECTOR_BYTES] = {};
    for (unsigned i = 0; i < APPLE_GCR_SECTOR_BYTES; ++i)
    {
        src[i] = static_cast<uint8_t>(i ^ 0xA5u);
    }
    REQUIRE(apple_gcr_encode_sector(src, nib) == 0);
    REQUIRE(apple_gcr_decode_sector(nib, dst) == 0);
    REQUIRE(std::memcmp(src, dst, APPLE_GCR_SECTOR_BYTES) == 0);
    REQUIRE(apple_gcr_encode_sector(nullptr, nib) == -1);
    REQUIRE(apple_gcr_decode_sector(nib, nullptr) == -1);
    nib[0] = 0x00;
    REQUIRE(apple_gcr_decode_sector(nib, dst) == -1);
}

TEST_CASE("Apple 6-and-2 empty and all-ones sectors", "[apple][gcr]")
{
    uint8_t z[APPLE_GCR_SECTOR_BYTES] = {};
    uint8_t f[APPLE_GCR_SECTOR_BYTES];
    std::memset(f, 0xFF, sizeof(f));
    uint8_t nib[APPLE_GCR_NIBBLE_BYTES] = {};
    uint8_t dst[APPLE_GCR_SECTOR_BYTES] = {};
    REQUIRE(apple_gcr_encode_sector(z, nib) == 0);
    REQUIRE(apple_gcr_decode_sector(nib, dst) == 0);
    REQUIRE(std::memcmp(z, dst, APPLE_GCR_SECTOR_BYTES) == 0);
    REQUIRE(apple_gcr_encode_sector(f, nib) == 0);
    REQUIRE(apple_gcr_decode_sector(nib, dst) == 0);
    REQUIRE(std::memcmp(f, dst, APPLE_GCR_SECTOR_BYTES) == 0);
}
