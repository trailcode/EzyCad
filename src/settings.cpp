#include "settings.h"

#include <cassert>
#include <fstream>
#include <functional>
#include <sstream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/fetch.h>
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
        var s = localStorage.getItem('ezycad_settings') || '';
        var len = lengthBytesUTF8(s) + 1;
        var buf = _malloc(len);
        if (buf) stringToUTF8(s, buf, len);
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
  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;
  emscripten_fetch_t* fetch = emscripten_fetch(&attr, "res/ezycad_settings.json");
  std::string result;
  if (fetch && fetch->data && fetch->numBytes > 0)
    result.assign(fetch->data, fetch->numBytes);
  if (fetch)
    emscripten_fetch_close(fetch);
  if (s_log_callback)
    s_log_callback(result.empty() ? "Settings: failed to load defaults"
                                  : "Settings: loaded defaults from res/ezycad_settings.json");
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
}
