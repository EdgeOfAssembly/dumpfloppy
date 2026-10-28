/**
 * @file test_tmp.hpp
 * @brief One private temp root per test run (mkdtemp under TMPDIR), removed at exit.
 *
 * Every test that writes files puts them under @ref tmp_root(). The root is
 * created on first use with mkdtemp(3) in std::filesystem::temp_directory_path()
 * (which honours TMPDIR), so parallel or repeated runs never share paths and
 * nothing is left under /tmp. A function-local static RAII guard removes the
 * whole tree when the process exits normally, including after failed
 * assertions (Catch2 returns from main). A crash (abort / sanitizer) skips it.
 */
#pragma once

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace dumpfloppy_test
{

/** @brief RAII owner of the per-run temp root. */
class tmp_root_guard
{
public:
    tmp_root_guard()
    {
        std::string tmpl = (std::filesystem::temp_directory_path() / "dumpfloppy-run-XXXXXX").string();
        std::vector<char> buf(tmpl.begin(), tmpl.end());
        buf.push_back('\0');
        if (::mkdtemp(buf.data()) == nullptr)
        {
            throw std::runtime_error("mkdtemp failed for " + tmpl);
        }
        path_ = std::filesystem::path(buf.data());
    }
    ~tmp_root_guard()
    {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }
    tmp_root_guard(const tmp_root_guard&) = delete;
    tmp_root_guard& operator=(const tmp_root_guard&) = delete;
    tmp_root_guard(tmp_root_guard&&) = delete;
    tmp_root_guard& operator=(tmp_root_guard&&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

/** @brief Unique per-run temp directory (created on first call, removed at exit). */
inline const std::filesystem::path& tmp_root()
{
    static const tmp_root_guard guard;
    return guard.path();
}

} /* namespace dumpfloppy_test */
