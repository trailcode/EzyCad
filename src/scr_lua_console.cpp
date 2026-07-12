#include "scr_lua_console.h"

#include "gui.h"
#include "imgui.h"
#include "mode.h"
#include "gui_occt_view.h"
#include "shp.h"
#include "shp_common.h"
#include "shp_cut.h"
#include "shp_fuse.h"
#include "sketch.h"
#include "utl_geom.h"

extern "C"
{
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
const char* k_registry_gui  = "ezycad_gui";
const char* k_shp_metatable = "EzyCad_Shp";

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

Shp_ptr* to_shp(lua_State* L, int index) { return static_cast<Shp_ptr*>(luaL_testudata(L, index, k_shp_metatable)); }

static Sketch_ref_plane parse_sketch_ref_plane(const char* plane)
{
  if (plane && (std::strcmp(plane, "XZ") == 0 || std::strcmp(plane, "xz") == 0))
    return Sketch_ref_plane::XZ;
  if (plane && (std::strcmp(plane, "YZ") == 0 || std::strcmp(plane, "yz") == 0))
    return Sketch_ref_plane::YZ;
  return Sketch_ref_plane::XY;
}

static const char* default_sketch_base_name(Sketch_ref_plane plane)
{
  switch (plane)
  {
  case Sketch_ref_plane::XZ:
    return "Sketch_xz";
  case Sketch_ref_plane::YZ:
    return "Sketch_yz";
  default:
    return "Sketch_xy";
  }
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

// view.curr_sketch_node_count() -> number
int l_view_curr_sketch_node_count(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
  {
    lua_pushinteger(L, 0);
    return 1;
  }
  lua_pushinteger(L, static_cast<lua_Integer>(view->curr_sketch().get_nodes().size()));
  return 1;
}

// view.curr_sketch_node(i) -> x, y (1-based index)
int l_view_curr_sketch_node(lua_State* L)
{
  GUI*        gui  = get_gui(L);
  Occt_view*  view = gui ? gui->get_view() : nullptr;
  lua_Integer idx  = luaL_checkinteger(L, 1);
  if (!view)
    return luaL_error(L, "no 3D view available");
  if (idx < 1)
    return luaL_error(L, "node index must be >= 1");
  Sketch_nodes& nodes = view->curr_sketch().get_nodes();
  if (static_cast<std::size_t>(idx) > nodes.size())
    return luaL_error(L, "node index out of range");
  const Sketch_nodes::Node& n = nodes[static_cast<std::size_t>(idx - 1)];
  lua_pushnumber(L, n.X());
  lua_pushnumber(L, n.Y());
  return 2;
}

// view.curr_sketch_dim_count() -> number
int l_view_curr_sketch_dim_count(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
  {
    lua_pushinteger(L, 0);
    return 1;
  }
  lua_pushinteger(L, static_cast<lua_Integer>(view->curr_sketch().length_dimension_count()));
  return 1;
}

// view.curr_sketch_dim(i) -> lo, hi, visible, offset, name, distance (1-based index)
int l_view_curr_sketch_dim(lua_State* L)
{
  GUI*        gui  = get_gui(L);
  Occt_view*  view = gui ? gui->get_view() : nullptr;
  lua_Integer idx  = luaL_checkinteger(L, 1);
  if (!view)
    return luaL_error(L, "no 3D view available");
  if (idx < 1)
    return luaL_error(L, "dimension index must be >= 1");
  Sketch& sketch = view->curr_sketch();
  if (static_cast<std::size_t>(idx) > sketch.length_dimension_count())
    return luaL_error(L, "dimension index out of range");
  const std::size_t   i     = static_cast<std::size_t>(idx - 1);
  const std::size_t   lo    = sketch.dimension_node_lo(i);
  const std::size_t   hi    = sketch.dimension_node_hi(i);
  const Sketch_nodes& nodes = sketch.get_nodes();
  const double        dist  = nodes[lo].Distance(nodes[hi]) / view->get_dimension_scale();
  lua_pushinteger(L, static_cast<lua_Integer>(lo + 1));
  lua_pushinteger(L, static_cast<lua_Integer>(hi + 1));
  lua_pushboolean(L, sketch.dimension_visible(i));
  lua_pushnumber(L, sketch.dimension_offset(i));
  lua_pushstring(L, sketch.dimension_name(i).c_str());
  lua_pushnumber(L, dist);
  return 6;
}

// view.add_sketch(plane, offset, base_name) - base_name optional
int l_view_add_sketch(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
    return luaL_error(L, "no 3D view available");
  const char*            plane  = luaL_optstring(L, 1, "XY");
  const double           offset = luaL_optnumber(L, 2, 0.0);
  const Sketch_ref_plane ref    = parse_sketch_ref_plane(plane);
  std::string            base   = default_sketch_base_name(ref);
  if (lua_gettop(L) >= 3 && !lua_isnil(L, 3))
    base = luaL_checkstring(L, 3);
  view->add_sketch_on_ref_plane(ref, offset, base);
  return 0;
}

// view.add_edge(x1, y1, x2, y2)
int l_view_add_edge(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
    return luaL_error(L, "no 3D view available");
  const double x1 = luaL_checknumber(L, 1);
  const double y1 = luaL_checknumber(L, 2);
  const double x2 = luaL_checknumber(L, 3);
  const double y2 = luaL_checknumber(L, 4);
  view->curr_sketch_add_edge(x1, y1, x2, y2);
  return 0;
}

// view.finish_sketch_edges()
int l_view_finish_sketch_edges(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
    return luaL_error(L, "no 3D view available");
  view->curr_sketch_rebuild_faces();
  return 0;
}

// view.add_box(ox, oy, oz, w, l, h) -> Shp
int l_view_add_box(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
  {
    lua_pushnil(L);
    return 1;
  }
  double ox  = luaL_checknumber(L, 1);
  double oy  = luaL_checknumber(L, 2);
  double oz  = luaL_checknumber(L, 3);
  double w   = luaL_checknumber(L, 4);
  double len = luaL_checknumber(L, 5);
  double h   = luaL_checknumber(L, 6);
  view->add_box(ox, oy, oz, w, len, h);
  std::list<Shp_ptr>& shapes = view->get_shapes();
  if (shapes.empty())
  {
    lua_pushnil(L);
    return 1;
  }
  push_shp(L, shapes.back());
  return 1;
}

// view.add_sphere(ox, oy, oz, radius) -> Shp
int l_view_add_sphere(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
  {
    lua_pushnil(L);
    return 1;
  }
  double ox = luaL_checknumber(L, 1);
  double oy = luaL_checknumber(L, 2);
  double oz = luaL_checknumber(L, 3);
  double r  = luaL_checknumber(L, 4);
  view->add_sphere(ox, oy, oz, r);
  std::list<Shp_ptr>& shapes = view->get_shapes();
  if (shapes.empty())
  {
    lua_pushnil(L);
    return 1;
  }
  push_shp(L, shapes.back());
  return 1;
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
  auto                it     = shapes.begin();
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

// view.fuse(s1, s2, ...) -> Shp
int l_view_fuse(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
    return luaL_error(L, "no 3D view available");
  const int n = lua_gettop(L);
  if (n < 2)
    return luaL_error(L, "fuse requires two or more shapes");
  std::vector<Shp_ptr> shps;
  shps.reserve(static_cast<std::size_t>(n));
  for (int i = 1; i <= n; ++i)
  {
    Shp_ptr* p = to_shp(L, i);
    if (!p || p->IsNull())
      return luaL_error(L, "fuse: argument %d must be a Shp", i);
    shps.push_back(*p);
  }
  Shp_rslt r = view->shp_fuse().fuse(std::move(shps));
  if (!r.is_ok())
    return luaL_error(L, "%s", r.message().empty() ? "fuse failed" : r.message().c_str());
  push_shp(L, r.value());
  return 1;
}

// view.cut(body, tool, ...) -> Shp
int l_view_cut(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
    return luaL_error(L, "no 3D view available");
  const int n = lua_gettop(L);
  if (n < 2)
    return luaL_error(L, "cut requires two or more shapes");
  std::vector<Shp_ptr> shps;
  shps.reserve(static_cast<std::size_t>(n));
  for (int i = 1; i <= n; ++i)
  {
    Shp_ptr* p = to_shp(L, i);
    if (!p || p->IsNull())
      return luaL_error(L, "cut: argument %d must be a Shp", i);
    shps.push_back(*p);
  }
  Shp_rslt r = view->shp_cut().cut(std::move(shps));
  if (!r.is_ok())
    return luaL_error(L, "%s", r.message().empty() ? "cut failed" : r.message().c_str());
  push_shp(L, r.value());
  return 1;
}

// view.common(s1, s2, ...) -> Shp
int l_view_common(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
    return luaL_error(L, "no 3D view available");
  const int n = lua_gettop(L);
  if (n < 2)
    return luaL_error(L, "common requires two or more shapes");
  std::vector<Shp_ptr> shps;
  shps.reserve(static_cast<std::size_t>(n));
  for (int i = 1; i <= n; ++i)
  {
    Shp_ptr* p = to_shp(L, i);
    if (!p || p->IsNull())
      return luaL_error(L, "common: argument %d must be a Shp", i);
    shps.push_back(*p);
  }
  Shp_rslt r = view->shp_common().common(std::move(shps));
  if (!r.is_ok())
    return luaL_error(L, "%s", r.message().empty() ? "common failed" : r.message().c_str());
  push_shp(L, r.value());
  return 1;
}

// view.delete(s1, ...)
int l_view_delete(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
    return luaL_error(L, "no 3D view available");
  const int n = lua_gettop(L);
  if (n < 1)
    return luaL_error(L, "delete requires one or more shapes");
  std::vector<AIS_Shape_ptr> to_delete;
  to_delete.reserve(static_cast<std::size_t>(n));
  for (int i = 1; i <= n; ++i)
  {
    Shp_ptr* p = to_shp(L, i);
    if (!p || p->IsNull())
      return luaL_error(L, "delete: argument %d must be a Shp", i);
    to_delete.push_back(*p);
  }
  view->delete_shapes(std::move(to_delete));
  return 0;
}

// view.get_camera() -> { eye={x,y,z}, center={x,y,z}, up={x,y,z} } or nil
int l_view_get_camera(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
  {
    lua_pushnil(L);
    return 1;
  }
  gp_Pnt eye;
  gp_Pnt center;
  gp_Dir up;
  if (!view->get_camera(eye, center, up))
  {
    lua_pushnil(L);
    return 1;
  }

  lua_newtable(L);

  lua_pushstring(L, "eye");
  lua_newtable(L);
  lua_pushnumber(L, eye.X());
  lua_rawseti(L, -2, 1);
  lua_pushnumber(L, eye.Y());
  lua_rawseti(L, -2, 2);
  lua_pushnumber(L, eye.Z());
  lua_rawseti(L, -2, 3);
  lua_settable(L, -3);

  lua_pushstring(L, "center");
  lua_newtable(L);
  lua_pushnumber(L, center.X());
  lua_rawseti(L, -2, 1);
  lua_pushnumber(L, center.Y());
  lua_rawseti(L, -2, 2);
  lua_pushnumber(L, center.Z());
  lua_rawseti(L, -2, 3);
  lua_settable(L, -3);

  lua_pushstring(L, "up");
  lua_newtable(L);
  lua_pushnumber(L, up.X());
  lua_rawseti(L, -2, 1);
  lua_pushnumber(L, up.Y());
  lua_rawseti(L, -2, 2);
  lua_pushnumber(L, up.Z());
  lua_rawseti(L, -2, 3);
  lua_settable(L, -3);

  return 1;
}

// view.set_camera(ex,ey,ez,cx,cy,cz,ux,uy,uz)
int l_view_set_camera(lua_State* L)
{
  GUI*       gui  = get_gui(L);
  Occt_view* view = gui ? gui->get_view() : nullptr;
  if (!view)
    return 0;
  const double ex = luaL_checknumber(L, 1);
  const double ey = luaL_checknumber(L, 2);
  const double ez = luaL_checknumber(L, 3);
  const double cx = luaL_checknumber(L, 4);
  const double cy = luaL_checknumber(L, 5);
  const double cz = luaL_checknumber(L, 6);
  const double ux = luaL_checknumber(L, 7);
  const double uy = luaL_checknumber(L, 8);
  const double uz = luaL_checknumber(L, 9);
  view->set_camera(gp_Pnt(ex, ey, ez), gp_Pnt(cx, cy, cz), gp_Dir(ux, uy, uz));
  return 0;
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
  Shp_ptr*    p = static_cast<Shp_ptr*>(luaL_checkudata(L, 1, k_shp_metatable));
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

// ezy.save_occt_view_settings()
int l_ezy_save_occt_view_settings(lua_State* L)
{
  GUI* gui = get_gui(L);
  if (gui)
    gui->save_occt_view_settings();
  return 0;
}

// ezy.occt_view_settings_json() -> string (occt_view object only)
int l_ezy_occt_view_settings_json(lua_State* L)
{
  GUI* gui = get_gui(L);
  if (!gui)
  {
    lua_pushstring(L, "{}");
    return 1;
  }
  const std::string s = gui->occt_view_settings_json();
  lua_pushlstring(L, s.data(), s.size());
  return 1;
}

// ezy.help() / help() - print available commands
int l_ezy_help(lua_State* L)
{
  lua_getfield(L, LUA_REGISTRYINDEX, "ezycad_console");
  Lua_console* con = static_cast<Lua_console*>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  if (!con)
    return 0;
  const char* help_text = "ezy (public scripting API):\n"
                          "  ezy.log(msg) / ezy.msg(text) / ezy.help()\n"
                          "  ezy.get_mode() / ezy.set_mode(name)\n"
                          "  ezy.save_occt_view_settings() / ezy.occt_view_settings_json()\n"
                          "ezy.view:\n"
                          "  sketch_count() / shape_count()\n"
                          "  add_box(ox,oy,oz,w,l,h) / add_sphere(ox,oy,oz,r)  - returns Shp\n"
                          "  fuse(s1, s2, ...)  - boolean union of two or more Shp; returns Shp\n"
                          "  cut(body, tool, ...)  - subtract tools from body; returns Shp\n"
                          "  common(s1, s2, ...)  - boolean intersection; returns Shp\n"
                          "  delete(s1, ...)  - remove one or more Shp from the document\n"
                          "  get_shape(i)  - shape by 1-based index (returns Shp or nil)\n"
                          "  get_camera() / set_camera(ex,ey,ez,cx,cy,cz,ux,uy,uz)\n"
                          "  curr_sketch.name() / node_count() / node(i) / dim_count() / dim(i)  (1-based indices)\n"
                          "ezy.sketch: (same table as ezy.view.curr_sketch)\n"
                          "  name() / node_count() / node(i) / dim_count() / dim(i)\n"
                          "  add(plane, offset, base_name) / add_edge(x1,y1,x2,y2) / finish_edges()\n"
                          "Shp: s:name() / set_name / visible / set_visible\n"
                          "aliases: view == ezy.view; help() == ezy.help()\n"
                          "  view.add_sketch / add_edge / finish_sketch_edges -> view.curr_sketch.*";
  con->append_line_from_lua(help_text);
  return 0;
}
} // namespace

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
  append_line("Lua console ready. Try: ezy.log('hello'), ezy.view.sketch_count(), view.curr_sketch.node_count()");
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
  lua_newtable(m_L); // __index
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

  // ezy.sketch table (also exposed as ezy.view.curr_sketch)
  lua_newtable(m_L);
  lua_pushcfunction(m_L, l_view_curr_sketch_name);
  lua_setfield(m_L, -2, "name");
  lua_pushcfunction(m_L, l_view_curr_sketch_name);
  lua_setfield(m_L, -2, "curr_name");
  lua_pushcfunction(m_L, l_view_curr_sketch_node_count);
  lua_setfield(m_L, -2, "node_count");
  lua_pushcfunction(m_L, l_view_curr_sketch_node);
  lua_setfield(m_L, -2, "node");
  lua_pushcfunction(m_L, l_view_curr_sketch_dim_count);
  lua_setfield(m_L, -2, "dim_count");
  lua_pushcfunction(m_L, l_view_curr_sketch_dim);
  lua_setfield(m_L, -2, "dim");
  lua_pushcfunction(m_L, l_view_add_sketch);
  lua_setfield(m_L, -2, "add");
  lua_pushcfunction(m_L, l_view_add_edge);
  lua_setfield(m_L, -2, "add_edge");
  lua_pushcfunction(m_L, l_view_finish_sketch_edges);
  lua_setfield(m_L, -2, "finish_edges");
  // stack: sketch

  // ezy.view table (document / shapes / camera)
  lua_newtable(m_L);
  lua_pushcfunction(m_L, l_view_sketch_count);
  lua_setfield(m_L, -2, "sketch_count");
  lua_pushcfunction(m_L, l_view_shape_count);
  lua_setfield(m_L, -2, "shape_count");
  lua_pushcfunction(m_L, l_view_add_box);
  lua_setfield(m_L, -2, "add_box");
  lua_pushcfunction(m_L, l_view_add_sphere);
  lua_setfield(m_L, -2, "add_sphere");
  lua_pushcfunction(m_L, l_view_fuse);
  lua_setfield(m_L, -2, "fuse");
  lua_pushcfunction(m_L, l_view_cut);
  lua_setfield(m_L, -2, "cut");
  lua_pushcfunction(m_L, l_view_common);
  lua_setfield(m_L, -2, "common");
  lua_pushcfunction(m_L, l_view_delete);
  lua_setfield(m_L, -2, "delete");
  lua_pushcfunction(m_L, l_view_get_shape);
  lua_setfield(m_L, -2, "get_shape");
  lua_pushcfunction(m_L, l_view_get_camera);
  lua_setfield(m_L, -2, "get_camera");
  lua_pushcfunction(m_L, l_view_set_camera);
  lua_setfield(m_L, -2, "set_camera");
  // view.curr_sketch -> same table as ezy.sketch
  lua_pushvalue(m_L, -2); // sketch
  lua_setfield(m_L, -2, "curr_sketch");
  // Compatibility aliases for sketch mutation
  lua_pushcfunction(m_L, l_view_add_sketch);
  lua_setfield(m_L, -2, "add_sketch");
  lua_pushcfunction(m_L, l_view_add_edge);
  lua_setfield(m_L, -2, "add_edge");
  lua_pushcfunction(m_L, l_view_finish_sketch_edges);
  lua_setfield(m_L, -2, "finish_sketch_edges");
  // stack: sketch, view

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
  lua_pushcfunction(m_L, l_ezy_save_occt_view_settings);
  lua_setfield(m_L, -2, "save_occt_view_settings");
  lua_pushcfunction(m_L, l_ezy_occt_view_settings_json);
  lua_setfield(m_L, -2, "occt_view_settings_json");
  // stack: sketch, view, ezy
  lua_pushvalue(m_L, -2); // view
  lua_setfield(m_L, -2, "view");
  lua_pushvalue(m_L, -3); // sketch
  lua_setfield(m_L, -2, "sketch");
  lua_setglobal(m_L, "ezy");
  // stack: sketch, view  -- set global view alias to same table as ezy.view
  lua_setglobal(m_L, "view");
  lua_pop(m_L, 1); // drop sketch (still reachable as ezy.sketch)

  // Global help() as well (same as ezy.help())
  lua_getglobal(m_L, "ezy");
  lua_getfield(m_L, -1, "help");
  lua_remove(m_L, -2);
  lua_setglobal(m_L, "help");

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

    std::string   content;
    std::ifstream f(path_str);
    if (f)
    {
      content.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
      f.close();
    }

    auto ed = std::make_unique<TextEditor>();
    ed->SetTabSize(2);
    ed->SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
    ed->SetText(content);

    m_script_editors.push_back({path_str, filename, std::move(ed)});

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

void Lua_console::render(bool* p_open)
{
  EZY_ASSERT(m_gui);

  ImFont* console_font = m_gui->console_font();

  if (!ImGui::Begin("Lua Console", p_open, ImGuiWindowFlags_None))
  {
    ImGui::End();
    return;
  }

  if (console_font)
    ImGui::PushFont(console_font);

  if (!ImGui::BeginTabBar("LuaConsoleTabs", ImGuiTabBarFlags_None))
  {
    if (console_font)
      ImGui::PopFont();
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
    bool run = ImGui::InputTextWithHint("##lua_input", "Enter Lua code (e.g. ezy.log('hi'))", m_input_buf, k_input_buf_size,
                                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory,
                                        &Lua_console::text_edit_callback, this);

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

    ImVec2 editor_size(-1.f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() - 4.f);
    if (script.editor && editor_size.y > 60.f)
    {
      ImGui::PushID(static_cast<int>(i));
      script.editor->Render("##script_body", editor_size, true);
      ImGui::PopID();
    }

    if (ImGui::Button("Save"))
    {
      std::ofstream of(script.path);
      if (of && script.editor)
      {
        const std::string text = script.editor->GetText();
        of.write(text.data(), static_cast<std::streamsize>(text.size()));
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

  if (console_font)
    ImGui::PopFont();
  ImGui::End();
}
