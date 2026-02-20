#pragma once

#include <string>

namespace settings
{
// Load persisted settings blob (INI-style text). Returns empty string if none.
std::string load();

// Save settings blob. Works on native (file) and Emscripten (localStorage).
void save(const std::string& content);
}
