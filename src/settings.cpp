#include "settings.h"

#include <cassert>
#include <fstream>
#include <functional>
#include <sstream>

#ifdef __EMSCRIPTEN__
// Defaults are loaded from preloaded file /res/ezycad_settings.json (no fetch).
#endif

namespace settings
{
static std::function<void(const std::string&)> s_log_callback;

void set_log_callback(std::function<void(const std::string&)> cb)
{
  s_log_callback = std::move(cb);
}

std::string load()
{
#ifdef __EMSCRIPTEN__
  // Stateless: never read from localStorage so every session uses defaults from server.
  return {};
#else
  std::ifstream f("ezycad_settings.json");
  if (!f)
    return {};
  std::ostringstream os;
  os << f.rdbuf();
  return os.str();
#endif
}

std::string load_defaults()
{
  if (s_log_callback)
    s_log_callback("Settings: loading defaults from res/ezycad_settings.json");
#ifdef __EMSCRIPTEN__
  // Read from preloaded file in virtual FS (--preload-file ...@/res/ezycad_settings.json).
  std::ifstream f("/res/ezycad_settings.json");
  if (!f)
  {
    if (s_log_callback)
      s_log_callback("Settings: failed to load defaults (preloaded file not found)");
    return {};
  }
  std::ostringstream os;
  os << f.rdbuf();
  std::string result = os.str();
  if (s_log_callback)
    s_log_callback("Settings: loaded defaults from res/ezycad_settings.json");
  return result;
#else
  std::ifstream f("res/ezycad_settings.json");
  assert(f && "res/ezycad_settings.json not found (required on native)");
  if (!f)
    return {};
  std::ostringstream os;
  os << f.rdbuf();
  std::string result = os.str();
  if (s_log_callback)
    s_log_callback("Settings: loaded defaults from res/ezycad_settings.json");
  return result;
#endif
}

std::string load_with_defaults()
{
  std::string content = load();
  if (content.empty())
  {
    content = load_defaults();
#ifndef __EMSCRIPTEN__
    if (!content.empty())
      save(content);
#endif
  }
  return content;
}

void save(const std::string& content)
{
#ifdef __EMSCRIPTEN__
  // Stateless: never write to localStorage.
  (void) content;
#else
  std::ofstream f("ezycad_settings.json");
  if (f)
    f << content;
#endif
}
}
