#include "settings.h"

#include <fstream>
#include <sstream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <cstdlib>
#endif

namespace settings
{
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
