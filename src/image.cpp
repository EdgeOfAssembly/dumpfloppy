/**
 * @file image.cpp
 * @brief Whole-file load of a floppy image (images are a few MiB at most).
 */
#include "dumpfloppy/image.hpp"
#include "dumpfloppy/geometry.hpp"
#include "dumpfloppy/util.hpp"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <system_error>

namespace dumpfloppy
{

std::expected<floppy_image, std::string>
load_image(const std::filesystem::path& path)
{
    std::error_code ec{};
    if (!std::filesystem::exists(path, ec))
    {
        return std::unexpected("cannot open '" + path.string() + "': no such file");
    }
    if (std::filesystem::is_directory(path, ec))
    {
        return std::unexpected("'" + path.string() + "' is a directory");
    }

    const uintmax_t sz = std::filesystem::file_size(path, ec);
    if (ec)
    {
        return std::unexpected("cannot stat '" + path.string() + "': " + ec.message());
    }
    if (sz == 0)
    {
        return std::unexpected("'" + path.string() + "' is empty");
    }
    if (sz > k_max_image_bytes)
    {
        return std::unexpected("'" + path.string() +
                               "' is larger than 16 MiB; not a floppy image");
    }

    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        return std::unexpected("cannot open '" + path.string() + "': " +
                               std::strerror(errno));
    }

    floppy_image img{};
    img.path = path;
    img.bytes.resize(static_cast<size_t>(sz));
    in.read(reinterpret_cast<char*>(img.bytes.data()),
            static_cast<std::streamsize>(sz));
    if (!in || in.gcount() != static_cast<std::streamsize>(sz))
    {
        return std::unexpected("short read of '" + path.string() + "'");
    }

    img.sha256 = sha256_hex(img.bytes);
    img.xxh64 = xxh64_hex(img.bytes);
    img.container = container_from_path(path.string());
    img.size_geometry = geometry_from_size(static_cast<uint64_t>(sz));
    return img;
}

} /* namespace dumpfloppy */
