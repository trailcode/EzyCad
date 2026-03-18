#include "lua_console.h"

#include "gui.h"
#include "imgui.h"
#include "modes.h"
#include "occt_view.h"
#include "shp.h"
#include "sketch.h"

extern "C"
{
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include <filesystem>
#include <fstream>
#include <cstring>
#include <sstream>
#include <vector>

namespace
{
const char* k_registry_gui       = "ezycad_gui";
const char* k_shp_metatable     = "EzyCad_Shp";

GUI* get_gui(lua_State* L)
{
  lua_getfield(L, LUA_REGISTRYINDEX, k_registry_gui);
  GUI* gui = static_cast<GUI*>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  return gui;
}

void push_shp(lua_State* L, const Shp_ptr& shp)
{
  void* block = lua_newuserdata(L, sizeof(Shp_ptr));
  new (block) Shp_ptr(shp);
  luaL_setmetatable(L, k_shp_metatable);
}

Shp_ptr* to_shp(lua_State* L, int index)
{
  return static_cast<Shp_ptr*>(luaL_testudata(L, index, k_shp_metatable));
}

// ezy.log(msg) -> append to console and log window
int l_ezy_log(lua_State* L)
{
  const char* msg = luaL_checkstring(L, 1);
  GUI*        gui = get_gui(L);
  if (gui)
    gui->log_message(msg);
  Lua_console* con = static_cast<Lua_console*>(lua_touserdata(L, lua_upvalueindex(1)));
  if (con)
    con->append_line_from_lua(msg);
  return 0;
}

// ezy.msg(text) -> show_message
int l_ezy_msg(lua_State* L)
{
  const char* text = luaL_checkstring(L, 1);
  GUI*        gui  = get_gui(L);
  if (gui)
    gui->show_message(text);
  return 0;
}

// ezy.get_mode() -> string
int l_ezy_get_mode(lua_State* L)
{
  GUI* gui = get_gui(L);
  if (!gui)
  {
    lua_pushstring(L, "Normal");
    return 1;
  }
  Mode m = gui->get_mode();
  lua_pushlstring(L, c_mode_strs[static_cast<int>(m)].data(), c_mode_strs[static_cast<int>(m)].size());
  return 1;
}

// ezy.set_mode(name)
int l_ezy_set_mode(lua_State* L)
{
  const char* name = luaL_checkstring(L, 1);
  GUI*        gui  = get_gui(L);
  if (gui)
    gui->set_mode(mode_from_string(name));
  return 0;
}

// view.sketch_count() -> number
int l_view_sketch_count(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
  {
    lua_pushinteger(L, 0);
    return 1;
  }
  lua_pushinteger(L, static_cast<lua_Integer>(view->get_sketches().size()));
  return 1;
}

// view.shape_count() -> number
int l_view_shape_count(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
  {
    lua_pushinteger(L, 0);
    return 1;
  }
  lua_pushinteger(L, static_cast<lua_Integer>(view->get_shapes().size()));
  return 1;
}

// view.curr_sketch_name() -> string
int l_view_curr_sketch_name(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
  {
    lua_pushstring(L, "");
    return 1;
  }
  lua_pushstring(L, view->curr_sketch().get_name().c_str());
  return 1;
}

// view.add_box(ox, oy, oz, w, l, h)
int l_view_add_box(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
    return 0;
  double ox  = luaL_checknumber(L, 1);
  double oy  = luaL_checknumber(L, 2);
  double oz  = luaL_checknumber(L, 3);
  double w   = luaL_checknumber(L, 4);
  double len = luaL_checknumber(L, 5);
  double h   = luaL_checknumber(L, 6);
  view->add_box(ox, oy, oz, w, len, h);
  return 0;
}

// view.add_sphere(ox, oy, oz, radius)
int l_view_add_sphere(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
    return 0;
  double ox = luaL_checknumber(L, 1);
  double oy = luaL_checknumber(L, 2);
  double oz = luaL_checknumber(L, 3);
  double r  = luaL_checknumber(L, 4);
  view->add_sphere(ox, oy, oz, r);
  return 0;
}

// view.get_shape(index) -> Shp userdata or nil (1-based index)
int l_view_get_shape(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
  {
    lua_pushnil(L);
    return 1;
  }
  lua_Integer idx = luaL_checkinteger(L, 1);
  if (idx < 1)
  {
    lua_pushnil(L);
    return 1;
  }
  std::list<Shp_ptr>& shapes = view->get_shapes();
  auto                it    = shapes.begin();
  for (lua_Integer i = 1; i < idx && it != shapes.end(); ++i, ++it)
    ;
  if (it == shapes.end())
  {
    lua_pushnil(L);
    return 1;
  }
  push_shp(L, *it);
  return 1;
}

// Shp userdata __gc
int l_shp_gc(lua_State* L)
{
  Shp_ptr* p = to_shp(L, 1);
  if (p)
    p->~Shp_ptr();
  return 0;
}

// Shp:name() -> string
int l_shp_name(lua_State* L)
{
  Shp_ptr* p = static_cast<Shp_ptr*>(luaL_checkudata(L, 1, k_shp_metatable));
  lua_pushstring(L, (*p)->get_name().c_str());
  return 1;
}

// Shp:set_name(s)
int l_shp_set_name(lua_State* L)
{
  Shp_ptr* p   = static_cast<Shp_ptr*>(luaL_checkudata(L, 1, k_shp_metatable));
  const char* s = luaL_checkstring(L, 2);
  (*p)->set_name(s);
  return 0;
}

// Shp:visible() -> boolean
int l_shp_visible(lua_State* L)
{
  Shp_ptr* p = static_cast<Shp_ptr*>(luaL_checkudata(L, 1, k_shp_metatable));
  lua_pushboolean(L, (*p)->get_visible());
  return 1;
}

// Shp:set_visible(b)
int l_shp_set_visible(lua_State* L)
{
  Shp_ptr* p = static_cast<Shp_ptr*>(luaL_checkudata(L, 1, k_shp_metatable));
  (*p)->set_visible(lua_toboolean(L, 2));
  return 0;
}

// ezy.help() / help() - print available commands
int l_ezy_help(lua_State* L)
{
  lua_getfield(L, LUA_REGISTRYINDEX, "ezycad_console");
  Lua_console* con = static_cast<Lua_console*>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  if (!con)
    return 0;
  const char* help_text =
      "ezy:\n"
      "  ezy.log(msg)          - append message to console and log window\n"
      "  ezy.msg(text)         - show status message\n"
      "  ezy.get_mode()        - return current mode name\n"
      "  ezy.set_mode(name)    - set mode by name\n"
      "  ezy.help()            - print this help\n"
      "view:\n"
      "  view.sketch_count()   - number of sketches\n"
      "  view.shape_count()    - number of shapes\n"
      "  view.curr_sketch_name() - current sketch name\n"
      "  view.add_box(ox,oy,oz,w,l,h) - add box\n"
      "  view.add_sphere(ox,oy,oz,r)  - add sphere\n"
  "  view.get_shape(i)            - get shape by 1-based index (returns Shp or nil)\n"
  "Shp (shape object):\n"
  "  s:name()       - get shape name\n"
  "  s:set_name(s)  - set shape name\n"
  "  s:visible()    - get visibility\n"
  "  s:set_visible(b) - set visibility";
  con->append_line_from_lua(help_text);
  return 0;
}
}  // namespace

// Called from C callback; we need to store Lua_console* in registry to get back
void Lua_console::append_line_from_lua(const std::string& line)
{
  m_history.push_back(line);
  m_scroll_to_bottom = true;
}

void Lua_console::append_line(const std::string& line, bool is_error)
{
  m_history.push_back(is_error ? "[err] " + line : line);
  m_scroll_to_bottom = true;
}

Lua_console::Lua_console(GUI* gui)
    : m_gui(gui)
{
  m_L = luaL_newstate();
  if (!m_L)
    return;
  luaL_openlibs(m_L);
  lua_pushlightuserdata(m_L, m_gui);
  lua_setfield(m_L, LUA_REGISTRYINDEX, k_registry_gui);
  lua_pushlightuserdata(m_L, this);
  lua_setfield(m_L, LUA_REGISTRYINDEX, "ezycad_console");
  register_bindings();
  load_scripts();
  append_line("Lua console ready. Try: ezy.log('hello'), ezy.get_mode(), view.sketch_count()");
}

Lua_console::~Lua_console()
{
  if (m_L)
    lua_close(m_L);
}

void Lua_console::register_bindings()
{
  if (!m_L || !m_gui)
    return;

  // Shp userdata metatable (in registry for luaL_setmetatable / luaL_checkudata)
  luaL_newmetatable(m_L, k_shp_metatable);
  lua_pushcfunction(m_L, l_shp_gc);
  lua_setfield(m_L, -2, "__gc");
  lua_newtable(m_L);  // __index
  lua_pushcfunction(m_L, l_shp_name);
  lua_setfield(m_L, -2, "name");
  lua_pushcfunction(m_L, l_shp_set_name);
  lua_setfield(m_L, -2, "set_name");
  lua_pushcfunction(m_L, l_shp_visible);
  lua_setfield(m_L, -2, "visible");
  lua_pushcfunction(m_L, l_shp_set_visible);
  lua_setfield(m_L, -2, "set_visible");
  lua_setfield(m_L, -2, "__index");
  lua_pop(m_L, 1);

  // ezy table
  lua_newtable(m_L);
  lua_pushlightuserdata(m_L, this);
  lua_pushcclosure(m_L, l_ezy_log, 1);
  lua_setfield(m_L, -2, "log");
  lua_pushcfunction(m_L, l_ezy_msg);
  lua_setfield(m_L, -2, "msg");
  lua_pushcfunction(m_L, l_ezy_get_mode);
  lua_setfield(m_L, -2, "get_mode");
  lua_pushcfunction(m_L, l_ezy_set_mode);
  lua_setfield(m_L, -2, "set_mode");
  lua_pushcfunction(m_L, l_ezy_help);
  lua_setfield(m_L, -2, "help");
  lua_setglobal(m_L, "ezy");

  // Global help() as well (same as ezy.help())
  lua_getglobal(m_L, "ezy");
  lua_getfield(m_L, -1, "help");
  lua_remove(m_L, -2);
  lua_setglobal(m_L, "help");

  // view table
  lua_newtable(m_L);
  lua_pushcfunction(m_L, l_view_sketch_count);
  lua_setfield(m_L, -2, "sketch_count");
  lua_pushcfunction(m_L, l_view_shape_count);
  lua_setfield(m_L, -2, "shape_count");
  lua_pushcfunction(m_L, l_view_curr_sketch_name);
  lua_setfield(m_L, -2, "curr_sketch_name");
  lua_pushcfunction(m_L, l_view_add_box);
  lua_setfield(m_L, -2, "add_box");
  lua_pushcfunction(m_L, l_view_add_sphere);
  lua_setfield(m_L, -2, "add_sphere");
  lua_pushcfunction(m_L, l_view_get_shape);
  lua_setfield(m_L, -2, "get_shape");
  lua_setglobal(m_L, "view");

  // Override print to use ezy.log
  lua_getglobal(m_L, "ezy");
  lua_getfield(m_L, -1, "log");
  lua_remove(m_L, -2);
  lua_setglobal(m_L, "print");
}

void Lua_console::load_scripts()
{
  if (!m_L)
    return;
#ifdef __EMSCRIPTEN__
  const std::filesystem::path scripts_dir("/res/scripts/lua");
#else
  const std::filesystem::path scripts_dir("res/scripts/lua");
#endif
  if (!std::filesystem::is_directory(scripts_dir))
    return;

  std::vector<std::filesystem::path> lua_files;
  for (const auto& entry : std::filesystem::directory_iterator(scripts_dir))
  {
    if (!entry.is_regular_file() || entry.path().extension() != ".lua")
      continue;
    lua_files.push_back(entry.path());
  }
  std::sort(lua_files.begin(), lua_files.end());

  for (const auto& path : lua_files)
  {
    std::string path_str = path.string();
    std::string filename = path.filename().string();

    std::string content;
    std::ifstream f(path_str);
    if (f)
    {
      content.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
      f.close();
    }

    m_script_editors.push_back({path_str, filename, std::move(content)});

    if (luaL_dofile(m_L, path_str.c_str()) != LUA_OK)
    {
      const char* err = lua_tostring(m_L, -1);
      append_line(std::string("script ") + filename + ": " + (err ? err : "?"), true);
      lua_pop(m_L, 1);
    }
  }
}

void Lua_console::execute(const std::string& code)
{
  if (!m_L || code.empty())
    return;

  // Track command history (avoid consecutive duplicates).
  if (!code.empty())
  {
    if (m_cmd_history.empty() || m_cmd_history.back() != code)
      m_cmd_history.push_back(code);
    m_cmd_history_pos = -1;
  }

  append_line("> " + code);
  if (luaL_loadstring(m_L, code.c_str()) != LUA_OK)
  {
    append_line(lua_tostring(m_L, -1), true);
    lua_pop(m_L, 1);
    return;
  }
  if (lua_pcall(m_L, 0, LUA_MULTRET, 0) != LUA_OK)
  {
    append_line(lua_tostring(m_L, -1), true);
    lua_pop(m_L, 1);
    return;
  }
  int n = lua_gettop(m_L);
  if (n > 0)
  {
    std::ostringstream oss;
    for (int i = 1; i <= n; ++i)
    {
      if (i > 1)
        oss << "\t";
      const char* s = lua_tostring(m_L, i);
      oss << (s ? s : lua_typename(m_L, lua_type(m_L, i)));
    }
    lua_pop(m_L, n);
    append_line(oss.str());
  }
}

int Lua_console::text_edit_callback(ImGuiInputTextCallbackData* data)
{
  auto* console = static_cast<Lua_console*>(data->UserData);
  if (!console)
    return 0;

  if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory && !console->m_cmd_history.empty())
  {
    const int history_size = static_cast<int>(console->m_cmd_history.size());

    if (data->EventKey == ImGuiKey_UpArrow)
    {
      if (console->m_cmd_history_pos == -1)
        console->m_cmd_history_pos = history_size - 1;
      else if (console->m_cmd_history_pos > 0)
        --console->m_cmd_history_pos;
    }
    else if (data->EventKey == ImGuiKey_DownArrow)
    {
      if (console->m_cmd_history_pos != -1)
      {
        if (console->m_cmd_history_pos + 1 < history_size)
          ++console->m_cmd_history_pos;
        else
          console->m_cmd_history_pos = -1;
      }
    }

    // Apply history entry (or clear when pos == -1).
    const char* new_buf = "";
    if (console->m_cmd_history_pos != -1)
      new_buf = console->m_cmd_history[console->m_cmd_history_pos].c_str();

    data->DeleteChars(0, data->BufTextLen);
    data->InsertChars(0, new_buf);
    data->SelectionStart = 0;
    data->SelectionEnd   = data->BufTextLen;
  }

  return 0;
}

int Lua_console::script_resize_callback(ImGuiInputTextCallbackData* data)
{
  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
  {
    auto* s = static_cast<std::string*>(data->UserData);
    s->resize(static_cast<size_t>(data->BufTextLen) + 1);
    data->Buf = s->data();
  }
  return 0;
}

void Lua_console::render(bool* p_open)
{
  if (!ImGui::Begin("Lua Console", p_open, ImGuiWindowFlags_None))
  {
    ImGui::End();
    return;
  }

  if (!ImGui::BeginTabBar("LuaConsoleTabs", ImGuiTabBarFlags_None))
  {
    ImGui::End();
    return;
  }

  if (ImGui::BeginTabItem("Console"))
  {
    float height = ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() - 4.f;
    if (height > 80.f)
    {
      constexpr size_t k_max_lines = 5000;
      constexpr size_t k_max_chars = 512 * 1024;
      std::string      display_text;
      display_text.reserve(m_history.size() * 32);
      size_t start_idx = m_history.size() > k_max_lines ? m_history.size() - k_max_lines : 0;
      for (size_t i = start_idx; i < m_history.size() && display_text.size() < k_max_chars; ++i)
      {
        display_text += m_history[i];
        display_text += '\n';
      }
      display_text += '\0';

      ImGui::BeginChild("Lua_consoleScroll", ImVec2(0, height), true, ImGuiWindowFlags_HorizontalScrollbar);
      const char* text_end = display_text.size() > 0 ? display_text.data() + display_text.size() - 1 : display_text.data();
      ImGui::TextUnformatted(display_text.data(), text_end);
      if (m_scroll_to_bottom)
      {
        ImGui::SetScrollHereY(1.0f);
        m_scroll_to_bottom = false;
      }
      ImGui::EndChild();
    }

    ImGui::SetNextItemWidth(-1);
    bool run = ImGui::InputTextWithHint("##lua_input",
                                        "Enter Lua code (e.g. ezy.log('hi'))",
                                        m_input_buf,
                                        k_input_buf_size,
                                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory,
                                        &Lua_console::text_edit_callback,
                                        this);

    if (run)
    {
      execute(m_input_buf);
      m_input_buf[0] = '\0';
      ImGui::SetKeyboardFocusHere(-1);
    }
    ImGui::EndTabItem();
  }

  for (size_t i = 0; i < m_script_editors.size(); ++i)
  {
    Script_editor& script = m_script_editors[i];
    if (!ImGui::BeginTabItem(script.filename.c_str()))
      continue;

    std::string& content = script.content;
    if (content.empty())
      content.resize(1, '\0');

    ImVec2 editor_size(-1.f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() - 4.f);
    if (editor_size.y > 60.f)
    {
      ImGui::InputTextMultiline(("##script_" + script.filename).c_str(),
                                content.data(),
                                content.size() + 1,
                                editor_size,
                                ImGuiInputTextFlags_CallbackResize,
                                &Lua_console::script_resize_callback,
                                &content);
    }

    if (ImGui::Button("Save"))
    {
      std::ofstream of(script.path);
      if (of)
      {
        size_t len = content.empty() ? 0 : (content.size() - (content.back() == '\0' ? 1 : 0));
        if (len > 0)
          of.write(content.data(), static_cast<std::streamsize>(len));
        if (of.good())
          append_line("Saved " + script.filename);
        else
          append_line("Save failed: " + script.filename, true);
      }
      else
        append_line("Could not open for write: " + script.path, true);
    }
    ImGui::EndTabItem();
  }

  ImGui::EndTabBar();
  ImGui::End();
}
