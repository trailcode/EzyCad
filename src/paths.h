#pragma once

#include <filesystem>
#include <optional>

namespace ezy
{
/// Directory containing the running executable, if detectable; empty on failure.
std::optional<std::filesystem::path> application_executable_directory();
}  // namespace ezy
