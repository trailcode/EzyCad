#pragma once

#include "ImGuiColorTextEdit/TextEditor.h"

#include <memory>
#include <string>
#include <vector>

class GUI;

/// One editable script file loaded from scripts/python (path, filename, TextEditor buffer).
struct Python_script_editor
{
  std::string path;
  std::string filename;
  TextEditor  editor;
};

/// ImGui Python console: run Python snippets with bindings to EzyCad (ezy.*, view.*).
/// Available when built with EZYCAD_HAVE_PYTHON (native builds with Python development libraries).
class Python_console
{
 public:
  explicit Python_console(GUI* gui);
  ~Python_console();

  void render(bool* p_open = nullptr);
  static int text_edit_callback(struct ImGuiInputTextCallbackData* data);
  static int log_display_resize_callback(struct ImGuiInputTextCallbackData* data);

  void append_line_from_python(const std::string& line);

 private:
  void load_scripts();
  void execute(const std::string& code);
  void append_line(const std::string& line, bool is_error = false);
  bool init_python_();

  GUI* m_gui = nullptr;

#ifdef EZYCAD_HAVE_PYTHON
  struct Ezycad_python_runtime;
  std::unique_ptr<Ezycad_python_runtime> m_py_runtime;
#endif

  std::vector<std::string>   m_history;
  std::vector<std::string>   m_cmd_history;
  int                        m_cmd_history_pos = -1;
  static constexpr int       k_input_buf_size  = 1024;
  char                       m_input_buf[k_input_buf_size] {};
  bool                       m_scroll_to_bottom = false;
  bool                       m_python_ok        = false;

  std::string m_log_display_buf;
  uint64_t    m_log_display_version       = 0;
  uint64_t    m_log_display_built_version = 0;

  std::vector<Python_script_editor> m_script_editors;
};

