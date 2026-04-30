#include "paths.h"

#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace ezy
{
std::optional<std::filesystem::path> application_executable_directory()
{
#if defined(_WIN32)
  std::vector<wchar_t> buf(MAX_PATH);
  for (;;)
  {
    DWORD n = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    if (n == 0)
      return std::nullopt;
    if (n < buf.size())
      return std::filesystem::path(buf.data()).parent_path();

    buf.resize(buf.size() * 2);
  }
#elif defined(__APPLE__)
  uint32_t sz = 0;
  _NSGetExecutablePath(nullptr, &sz);
  if (sz == 0)
    return std::nullopt;
  std::vector<char> buf(sz);
  if (_NSGetExecutablePath(buf.data(), &sz) != 0)
    return std::nullopt;
  std::error_code ec;
  auto            canon = std::filesystem::canonical(std::string(buf.data()), ec);
  if (ec)
    return std::nullopt;
  return canon.parent_path();
#elif defined(__linux__)
  std::error_code ec;
  auto            link = std::filesystem::read_symlink("/proc/self/exe", ec);
  if (ec)
    return std::nullopt;
  return link.parent_path();
#else
  return std::nullopt;
#endif
}

}  // namespace ezy
