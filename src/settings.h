#pragma once

#include <functional>
#include <string>

namespace settings {
// Optional: set a callback to receive log messages from load_defaults() (e.g. GUI log window).
void set_log_callback(std::function<void(const std::string&)> cb);

// Load persisted settings blob. Returns empty string if none.
std::string load();

// Load default settings. Native: from res/ezycad_settings.json (asserts if missing).
// Emscripten: from server (same-origin res/ezycad_settings.json).
std::string load_defaults();

// Load settings, or defaults if none. If defaults were used, they are saved so settings exist next time.
std::string load_with_defaults();

// Save settings blob. Works on native (file) and Emscripten (localStorage).
void save(const std::string& content);
}  // namespace settings
