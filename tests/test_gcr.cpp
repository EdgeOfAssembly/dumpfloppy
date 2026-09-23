/**
 * @file test_gcr.cpp
 * @brief Commodore 4-to-5 GCR nibble/byte codec.
 */
#include "dumpfloppy/gcr_codec.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("GCR nibble encode/decode round-trip", "[gcr]")
{
    for (unsigned n = 0; n < 16u; ++n)
    {
        const uint8_t five = gcr_encode_nibble(static_cast<uint8_t>(n));
        REQUIRE((five & static_cast<uint8_t>(~GCR_FIVE_MASK)) == 0u);
        REQUIRE(gcr_decode_nibble(five) == static_cast<uint8_t>(n));
    }
}

TEST_CASE("GCR unused 5-bit groups are invalid", "[gcr]")
{
    REQUIRE(gcr_decode_nibble(0x00) == GCR_INVALID);
    REQUIRE(gcr_decode_nibble(0x1C) == GCR_INVALID);
    REQUIRE(gcr_decode_nibble(0x08) == GCR_INVALID);
}

TEST_CASE("GCR textbook codes", "[gcr]")
{
    REQUIRE(gcr_encode_nibble(0x0) == 0x0A);
    REQUIRE(gcr_encode_nibble(0x8) == 0x09);
    REQUIRE(gcr_encode_nibble(0xF) == 0x15);
    REQUIRE(gcr_encode_nibble(0xA) == 0x1A);
    REQUIRE(gcr_decode_nibble(0x1F) == GCR_INVALID);
}

TEST_CASE("GCR byte round-trip and NULL out", "[gcr]")
{
    for (unsigned b = 0; b < 256u; ++b)
    {
        uint8_t hi = 0;
        uint8_t lo = 0;
        uint8_t out = 0xCC;
        gcr_encode_byte(static_cast<uint8_t>(b), &hi, &lo);
        REQUIRE(gcr_decode_byte(hi, lo, &out) == 0);
        REQUIRE(out == static_cast<uint8_t>(b));
    }
    uint8_t hi = 0;
    uint8_t lo = 0;
    gcr_encode_byte(0x08, &hi, &lo);
    REQUIRE(gcr_decode_byte(hi, lo, nullptr) == -1);
    REQUIRE(gcr_decode_byte(0x00, 0x0A, &hi) == -1);
}
