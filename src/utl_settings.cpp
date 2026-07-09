#include "utl_settings.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

#include <cstdlib>

#include "utl_io.h"
#endif

namespace settings
{
static std::function<void(const std::string&)> s_log_callback;

void set_log_callback(std::function<void(const std::string&)> cb) { s_log_callback = std::move(cb); }

// Portable getenv wrapper: uses _dupenv_s on MSVC to avoid C4996 deprecation;
// falls back to std::getenv elsewhere. Returns empty string on failure/missing.
static std::string get_env_var(const char* name)
{
#ifdef _MSC_VER
  char*  buf = nullptr;
  size_t sz  = 0;
  if (_dupenv_s(&buf, &sz, name) == 0 && buf != nullptr)
  {
    std::string val(buf);
    free(buf);
    return val;
  }

  return {};
#else
  const char* v = std::getenv(name);
  return v ? std::string(v) : std::string{};
#endif
}

std::filesystem::path user_settings_json_path()
{
#ifdef __EMSCRIPTEN__
  return {};
#else
  const std::filesystem::path dir = user_config_directory();
  if (dir.empty())
    return {};

  return dir / "ezycad_settings.json";
#endif
}

std::string load_with_defaults()
{
  std::string content;
#ifdef __EMSCRIPTEN__
  void* ptr = (void*)(intptr_t)EM_ASM_INT({
    var s   = localStorage.getItem("ezycad_settings") || "";
    var len = lengthBytesUTF8(s) + 1;
    var buf = _malloc(len);
    if (buf)
      stringToUTF8(s, buf, len);
    return buf;
  });
  if (ptr)
  {
    content = (const char*)ptr;
    free(ptr);
  }
#else
  auto read_file = [](const std::filesystem::path& p) -> std::string
  {
    std::ifstream f(p, std::ios::binary);
    if (!f)
      return {};

    std::ostringstream os;
    os << f.rdbuf();
    return os.str();
  };

  const std::filesystem::path user_p = user_settings_json_path();
  if (!user_p.empty())
    content = read_file(user_p);
  if (content.empty())
  {
    // Legacy: cwd ezycad_settings.json (same directory as exe when launched that way).
    content = read_file(std::filesystem::path("ezycad_settings.json"));
  }
#endif
  if (content.empty())
  {
    content = load_defaults();
    if (!content.empty())
      save(content);
  }
  return content;
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

void save(const std::string& content)
{
#ifdef __EMSCRIPTEN__
  EM_ASM_({ localStorage.setItem('ezycad_settings', UTF8ToString($0)); }, content.c_str());
#else
  const std::filesystem::path user_p = user_settings_json_path();
  if (!user_p.empty())
  {
    std::error_code ec;
    std::filesystem::create_directories(user_p.parent_path(), ec);
    std::ofstream f(user_p, std::ios::binary);
    if (f)
    {
      f.write(content.data(), static_cast<std::streamsize>(content.size()));
      return;
    }
  }
  std::ofstream f("ezycad_settings.json", std::ios::binary);
  if (f)
    f.write(content.data(), static_cast<std::streamsize>(content.size()));
#endif
}

std::filesystem::path user_config_directory()
{
#ifdef __EMSCRIPTEN__
  (void)0;
  return {};
#elif defined(_WIN32)
  std::string appdata = get_env_var("APPDATA");
  if (appdata.empty())
    return {};

  return std::filesystem::path(appdata) / "EzyCad";
#elif defined(__APPLE__)
  std::string home = get_env_var("HOME");
  if (home.empty())
    return {};

  return std::filesystem::path(home) / "Library" / "Application Support" / "EzyCad";
#else
  std::string xdg = get_env_var("XDG_CONFIG_HOME");
  if (!xdg.empty())
    return std::filesystem::path(xdg) / "EzyCad";

  std::string home = get_env_var("HOME");
  if (home.empty())
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
  void* ptr = (void*)(intptr_t)EM_ASM_INT({
    var s   = localStorage.getItem("ezycad_startup_ezy") || "";
    var len = lengthBytesUTF8(s) + 1;
    var buf = _malloc(len);
    if (buf)
      stringToUTF8(s, buf, len);

    return buf;
  });
  if (!ptr)
    return {};

  std::string stored((const char*)ptr);
  free(ptr);
  if (stored.rfind("B64:", 0) == 0)
  {
    const std::vector<uint8_t> decoded = ezy_base64_decode(stored.substr(4));
    return std::string(reinterpret_cast<const char*>(decoded.data()), decoded.size());
  }

  return stored;
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

bool save_user_startup_project(const std::vector<uint8_t>& ezy_bytes)
{
#ifdef __EMSCRIPTEN__
  const std::string b64 = std::string("B64:") + ezy_base64_encode(ezy_bytes);
  EM_ASM_({ localStorage.setItem('ezycad_startup_ezy', UTF8ToString($0)); }, b64.c_str());
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

  f.write(reinterpret_cast<const char*>(ezy_bytes.data()), static_cast<std::streamsize>(ezy_bytes.size()));
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
} // namespace settings
