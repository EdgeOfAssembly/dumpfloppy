/**
 * @file util.cpp
 * @brief String, timestamp, and SHA-256 helpers.
 */
#include "dumpfloppy/util.hpp"

#include <array>
#include <cstdio>
#include <openssl/sha.h>

namespace dumpfloppy
{

std::string ascii_field(std::span<const uint8_t> data, size_t off, size_t n)
{
    if (!in_range(data, off, n))
    {
        return {};
    }
    std::string out;
    out.reserve(n);
    for (size_t i = 0; i < n; ++i)
    {
        const uint8_t b = data[off + i];
        if (b == 0u)
        {
            break;
        }
        out.push_back(static_cast<char>(b));
    }
    while (!out.empty() && (out.back() == ' ' || out.back() == '\0'))
    {
        out.pop_back();
    }
    return out;
}

bool is_printable_ascii(std::string_view s)
{
    if (s.empty())
    {
        return false;
    }
    for (const char c : s)
    {
        const unsigned char u = static_cast<unsigned char>(c);
        if (u < 0x20u || u > 0x7Eu)
        {
            return false;
        }
    }
    return true;
}

std::string format_volume_serial(uint32_t serial)
{
    char buf[16] = {};
    const int n = std::snprintf(buf, sizeof(buf), "%04X-%04X",
                                static_cast<unsigned>((serial >> 16) & 0xFFFFu),
                                static_cast<unsigned>(serial & 0xFFFFu));
    if (n < 0)
    {
        return {};
    }
    return std::string(buf);
}

std::string format_dos_date(uint16_t dos_date)
{
    if (dos_date == 0u)
    {
        return "----/--/--";
    }
    const unsigned day = static_cast<unsigned>(dos_date & 0x1Fu);
    const unsigned month = static_cast<unsigned>((dos_date >> 5) & 0x0Fu);
    const unsigned year = 1980u + static_cast<unsigned>((dos_date >> 9) & 0x7Fu);
    char buf[16] = {};
    const int n = std::snprintf(buf, sizeof(buf), "%04u-%02u-%02u", year, month, day);
    if (n < 0)
    {
        return {};
    }
    return std::string(buf);
}

std::string format_dos_time(uint16_t dos_time)
{
    if (dos_time == 0u)
    {
        return "--:--:--";
    }
    const unsigned sec = static_cast<unsigned>(dos_time & 0x1Fu) * 2u;
    const unsigned min = static_cast<unsigned>((dos_time >> 5) & 0x3Fu);
    const unsigned hour = static_cast<unsigned>((dos_time >> 11) & 0x1Fu);
    char buf[16] = {};
    const int n = std::snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hour, min, sec);
    if (n < 0)
    {
        return {};
    }
    return std::string(buf);
}

std::string ascii_lower(std::string_view s)
{
    std::string out(s);
    for (char& c : out)
    {
        if (c >= 'A' && c <= 'Z')
        {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return out;
}

std::string sha256_hex(std::span<const uint8_t> data)
{
    std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
    SHA256(data.data(), data.size(), digest.data());
    std::string hex;
    hex.resize(static_cast<size_t>(SHA256_DIGEST_LENGTH) * 2u);
    static constexpr char k_digits[] = "0123456789abcdef";
    for (size_t i = 0; i < digest.size(); ++i)
    {
        hex[i * 2u] = k_digits[(digest[i] >> 4) & 0x0Fu];
        hex[i * 2u + 1u] = k_digits[digest[i] & 0x0Fu];
    }
    return hex;
}

std::vector<std::string> printable_runs(std::span<const uint8_t> data, size_t min_len)
{
    std::vector<std::string> runs;
    size_t i = 0;
    const size_t n = data.size();
    while (i < n)
    {
        if (data[i] >= 0x20u && data[i] <= 0x7Eu)
        {
            const size_t start = i;
            while (i < n && data[i] >= 0x20u && data[i] <= 0x7Eu)
            {
                ++i;
            }
            if (i - start >= min_len)
            {
                runs.emplace_back(reinterpret_cast<const char*>(data.data() + start),
                                  i - start);
            }
        }
        else
        {
            ++i;
        }
    }
    return runs;
}

} /* namespace dumpfloppy */
