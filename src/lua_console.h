#pragma once

#include <memory>
#include <string>
#include <vector>

struct lua_State;
class GUI;

/// One editable script file loaded from scripts/lua (path, filename, content).
struct Script_editor
{
  std::string path;      // full path for load/save
  std::string filename; // tab title (e.g. "basic.lua")
  std::string content;   // editable source
};

/// ImGui Lua console: run Lua snippets with bindings to EzyCad (ezy.*, view.*).
/// Built for native and Emscripten (Lua C sources compile with emcc).
class Lua_console
{
 public:
  explicit Lua_console(GUI* gui);
  ~Lua_console();

  void render(bool* p_open = nullptr);
  static int text_edit_callback(struct ImGuiInputTextCallbackData* data);
  static int script_resize_callback(struct ImGuiInputTextCallbackData* data);

  /// Called from Lua ezy.log(); appends to console history.
  void append_line_from_lua(const std::string& line);

 private:
  void register_bindings();
  void load_scripts();
  void execute(const std::string& code);
  void append_line(const std::string& line, bool is_error = false);

  GUI*       m_gui = nullptr;
  lua_State* m_L   = nullptr;

  std::vector<std::string>   m_history;        // output lines (result or error)
  std::vector<std::string>   m_cmd_history;    // entered Lua commands
  int                        m_cmd_history_pos = -1;  // -1 when not browsing
  static constexpr int       k_input_buf_size  = 1024;
  char                       m_input_buf[k_input_buf_size] {};
  bool                       m_scroll_to_bottom = false;

  std::vector<Script_editor> m_script_editors;  // one per .lua in scripts/lua
};
