/**
 * @file identify.cpp
 * @brief libmagic cookie, opened once per process.
 */
#include "dumpfloppy/identify.hpp"

#include <magic.h>

namespace dumpfloppy
{
namespace
{

magic_t cookie()
{
    static magic_t m = nullptr;
    static bool tried = false;
    if (!tried)
    {
        tried = true;
        m = magic_open(MAGIC_MIME_TYPE);
        if (m != nullptr && magic_load(m, nullptr) != 0)
        {
            magic_close(m);
            m = nullptr;
        }
    }
    return m;
}

} /* namespace */

std::string mime_type(std::span<const uint8_t> data)
{
    if (data.empty())
    {
        return "application/x-empty";
    }
    magic_t m = cookie();
    if (m == nullptr)
    {
        return "application/octet-stream";
    }
    const char* s = magic_buffer(m, data.data(), data.size());
    if (s == nullptr || s[0] == '\0')
    {
        return "application/octet-stream";
    }
    return std::string(s);
}

} /* namespace dumpfloppy */
