#pragma once

#include <memory>
#include <string>
#include <vector>

class GUI;

#ifdef __EMSCRIPTEN__
/// Stub when building for Emscripten (Lua console not available in web build).
class Lua_console
{
 public:
  explicit Lua_console(GUI*) {}
  void render(bool* = nullptr) {}
  void append_line_from_lua(const std::string&) {}
};
#else
struct lua_State;

/// ImGui Lua console: run Lua snippets with bindings to EzyCad (ezy.*, view.*).
class Lua_console
{
 public:
  explicit Lua_console(GUI* gui);
  ~Lua_console();

  void render(bool* p_open = nullptr);

  /// Called from Lua ezy.log(); appends to console history.
  void append_line_from_lua(const std::string& line);

 private:
  void register_bindings();
  void execute(const std::string& code);
  void append_line(const std::string& line, bool is_error = false);

  GUI*       m_gui = nullptr;
  lua_State* m_L   = nullptr;

  std::vector<std::string> m_history;   // output lines (result or error)
  static constexpr int     k_input_buf_size = 1024;
  char                     m_input_buf[k_input_buf_size] {};
  bool                     m_scroll_to_bottom = false;
};
#endif
