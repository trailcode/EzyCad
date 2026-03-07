#include "lua_console.h"

#include "gui.h"
#include "imgui.h"
#include "modes.h"
#include "occt_view.h"
#include "sketch.h"

extern "C"
{
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include <cstring>
#include <sstream>

namespace
{
const char* k_registry_gui = "ezycad_gui";

GUI* get_gui(lua_State* L)
{
  lua_getfield(L, LUA_REGISTRYINDEX, k_registry_gui);
  GUI* gui = static_cast<GUI*>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  return gui;
}

// ezy.log(msg) -> append to console and log window
int l_ezy_log(lua_State* L)
{
  const char* msg = luaL_checkstring(L, 1);
  GUI*        gui = get_gui(L);
  if (gui)
    gui->log_message(msg);
  LuaConsole* con = static_cast<LuaConsole*>(lua_touserdata(L, lua_upvalueindex(1)));
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
}  // namespace

// Called from C callback; we need to store LuaConsole* in registry to get back
void LuaConsole::append_line_from_lua(const std::string& line)
{
  m_history.push_back(line);
  m_scroll_to_bottom = true;
}

void LuaConsole::append_line(const std::string& line, bool is_error)
{
  m_history.push_back(is_error ? "[err] " + line : line);
  m_scroll_to_bottom = true;
}

LuaConsole::LuaConsole(GUI* gui)
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
  append_line("Lua console ready. Try: ezy.log('hello'), ezy.get_mode(), view.sketch_count()");
}

LuaConsole::~LuaConsole()
{
  if (m_L)
    lua_close(m_L);
}

void LuaConsole::register_bindings()
{
  if (!m_L || !m_gui)
    return;

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
  lua_setglobal(m_L, "ezy");

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
  lua_setglobal(m_L, "view");

  // Override print to use ezy.log
  lua_getglobal(m_L, "ezy");
  lua_getfield(m_L, -1, "log");
  lua_remove(m_L, -2);
  lua_setglobal(m_L, "print");
}

void LuaConsole::execute(const std::string& code)
{
  if (!m_L || code.empty())
    return;
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

void LuaConsole::render(bool* p_open)
{
  if (!ImGui::Begin("Lua Console", p_open, ImGuiWindowFlags_None))
  {
    ImGui::End();
    return;
  }

  float height = ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() - 4.f;
  if (height > 80.f)
  {
    ImGui::BeginChild("LuaConsoleScroll", ImVec2(0, height), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& line : m_history)
      ImGui::TextUnformatted(line.c_str());
    if (m_scroll_to_bottom)
    {
      ImGui::SetScrollHereY(1.0f);
      m_scroll_to_bottom = false;
    }
    ImGui::EndChild();
  }

  ImGui::SetNextItemWidth(-1);
  bool run = ImGui::InputTextWithHint("##lua_input", "Enter Lua code (e.g. ezy.log('hi'))", m_input_buf, k_input_buf_size, ImGuiInputTextFlags_EnterReturnsTrue);
  if (run)
  {
    execute(m_input_buf);
    m_input_buf[0] = '\0';
  }
  ImGui::End();
}
