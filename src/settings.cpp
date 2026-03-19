#include "settings.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

#include <cstdlib>
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
  void* ptr = (void*) (intptr_t) EM_ASM_INT(
      {
        var s   = localStorage.getItem('ezycad_settings') || '';
        var len = lengthBytesUTF8(s) + 1;
        var buf = _malloc(len);
        if (buf)
          stringToUTF8(s, buf, len);
        return buf;
      });
  if (!ptr)
    return {};
  std::string result((const char*) ptr);
  free(ptr);
  return result;
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
    if (!content.empty())
      save(content);
  }
  return content;
}

void save(const std::string& content)
{
#ifdef __EMSCRIPTEN__
  EM_ASM_({ localStorage.setItem('ezycad_settings', UTF8ToString($0)); }, content.c_str());
#else
  std::ofstream f("ezycad_settings.json");
  if (f)
    f << content;
#endif
}

std::filesystem::path user_config_directory()
{
#ifdef __EMSCRIPTEN__
  (void) 0;
  return {};
#elif defined(_WIN32)
  const char* appdata = std::getenv("APPDATA");
  if (!appdata || !*appdata)
    return {};
  return std::filesystem::path(appdata) / "EzyCad";
#elif defined(__APPLE__)
  const char* home = std::getenv("HOME");
  if (!home || !*home)
    return {};
  return std::filesystem::path(home) / "Library" / "Application Support" / "EzyCad";
#else
  const char* xdg = std::getenv("XDG_CONFIG_HOME");
  if (xdg && *xdg)
    return std::filesystem::path(xdg) / "EzyCad";
  const char* home = std::getenv("HOME");
  if (!home || !*home)
    return {};
  return std::filesystem::path(home) / ".config" / "EzyCad";
#endif
}

std::filesystem::path user_startup_project_path()
{
  const std::filesystem::path dir = user_config_directory();
  if (dir.empty())
    return {};
  return dir / "startup.ezy";
}

std::string load_user_startup_project()
{
#ifdef __EMSCRIPTEN__
  void* ptr = (void*) (intptr_t) EM_ASM_INT(
      {
        var s   = localStorage.getItem('ezycad_startup_ezy') || '';
        var len = lengthBytesUTF8(s) + 1;
        var buf = _malloc(len);
        if (buf)
          stringToUTF8(s, buf, len);
        return buf;
      });
  if (!ptr)
    return {};
  std::string result((const char*) ptr);
  free(ptr);
  return result;
#else
  const std::filesystem::path p = user_startup_project_path();
  if (p.empty())
    return {};
  std::ifstream f(p, std::ios::binary);
  if (!f)
    return {};
  std::ostringstream os;
  os << f.rdbuf();
  return os.str();
#endif
}

bool save_user_startup_project(const std::string& json)
{
#ifdef __EMSCRIPTEN__
  EM_ASM_({ localStorage.setItem('ezycad_startup_ezy', UTF8ToString($0)); }, json.c_str());
  return true;
#else
  const std::filesystem::path p = user_startup_project_path();
  if (p.empty())
    return false;
  std::error_code ec;
  std::filesystem::create_directories(p.parent_path(), ec);
  if (ec)
    return false;
  std::ofstream f(p, std::ios::binary);
  if (!f)
    return false;
  f.write(json.data(), static_cast<std::streamsize>(json.size()));
  return true;
#endif
}

void clear_user_startup_project()
{
#ifdef __EMSCRIPTEN__
  EM_ASM({ localStorage.removeItem('ezycad_startup_ezy'); });
#else
  const std::filesystem::path p = user_startup_project_path();
  if (!p.empty())
  {
    std::error_code ec;
    std::filesystem::remove(p, ec);
  }
#endif
}
}  // namespace settings
