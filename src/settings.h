#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace settings
{
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

// User config dir: %APPDATA%/EzyCad (Windows), ~/Library/Application Support/EzyCad (macOS),
// $XDG_CONFIG_HOME/EzyCad or ~/.config/EzyCad (Linux). Empty if unavailable.
std::filesystem::path user_config_directory();

// Path to the user "startup" project (.../startup.ezy). Empty if user_config_directory() is empty.
std::filesystem::path user_startup_project_path();

// Optional startup project (Blender-style). Native: user_startup_project_path(); Wasm: localStorage.
std::string load_user_startup_project();
bool        save_user_startup_project(const std::string& json);
void        clear_user_startup_project();
}  // namespace settings
