#pragma once

#include <chrono>  // For message status window (from previous request)
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>  // Added for log messages
#include <variant>
#include <vector>  // Added for log storage

#include "modes.h"
#include "shp.h"
#include "types.h"

class Occt_view;
struct GLFWwindow;

enum class Command
{
  Shape_cut,
  Shape_fuse,
  Shape_common,
  _count
};

class GUI
{
 public:
  GUI();
  ~GUI();

#ifdef __EMSCRIPTEN__
  static GUI& instance();
#endif

  void init(GLFWwindow* window);

  void render_gui();
  void render_occt();

  void on_key(int key, int scancode, int action, int mods);
  void on_mouse_pos(const ScreenCoords& screen_coords);
  void on_mouse_button(int button, int action, int mods);
  void on_mouse_scroll(double xoffset, double yoffset);
  void on_resize(int width, int height);

  // clang-format off
  Mode get_mode() const { return m_mode; }
  Chamfer_mode get_chamfer_mode() const { return m_chamfer_mode; }
  void set_mode(Mode mode);
  void set_parent_mode();
  void set_dist_edit(float dist, std::function<void(float, bool)>&& callback, const std::optional<ScreenCoords> screen_coords = std::nullopt);
  void hide_dist_edit();
  void show_message(const std::string& message);
  void log_message(const std::string& message);
  // clang-format on

#ifdef __EMSCRIPTEN__
  void open_file_dialog_async(const char* title);  // Async for Emscripten
  void save_file_dialog_async(const char* title, const std::string& default_file, const std::string& json_str);
#endif

  void on_file(const std::string& file_path, const std::string& json_str);
  void on_import_file(const std::string& file_path, const std::string& file_data);

  // private:
  friend class GUI_access;

  // Structure to hold button state and texture
  struct Toolbar_button
  {
    uint32_t                    texture_id;
    bool                        is_active;
    const char*                 tooltip;
    std::variant<Mode, Command> data;
  };

  // Custom stream buffer to redirect stdout/stderr to log_message
  class LogStreamBuf : public std::streambuf
  {
   public:
    LogStreamBuf(GUI& gui, std::streambuf* original_buf)
        : m_gui(gui), m_original_buf(original_buf) {}

   protected:
    int overflow(int c) override
    {
      if (c != EOF)
      {
        m_buffer += static_cast<char>(c);
        if (c == '\n')
        {
          // Remove trailing newline for log_message
          if (!m_buffer.empty() && m_buffer.back() == '\n')
            m_buffer.pop_back();
          if (!m_buffer.empty())
            m_gui.log_message(m_buffer);
          m_buffer.clear();
        }
        // Forward to original console
        if (m_original_buf)
          m_original_buf->sputc(c);
      }
      return c;
    }

    int sync() override
    {
      if (!m_buffer.empty())
      {
        m_gui.log_message(m_buffer);
        m_buffer.clear();
      }
      return m_original_buf ? m_original_buf->pubsync() : 0;
    }

   private:
    GUI&            m_gui;
    std::streambuf* m_original_buf;
    std::string     m_buffer;
  };

  void dist_edit_();
  void sketch_list_();
  void shape_list_();
  void options_();
  void options_normal_mode_();
  void options_move_mode_();
  void on_key_move_mode_(int key);
  void on_key_rotate_mode_(int key);
  void options_sketch_operation_axis_mode_();
  void options_shape_chamfer_mode_();
  void options_shape_polar_duplicate_mode_();
  void options_rotate_mode_();
  void dbg_();
  void initialize_toolbar_();
  void menu_bar_();
  void toolbar_();
  void message_status_window_();
  void log_window_();
  void setup_log_redirection_();
  void cleanup_log_redirection_();

  void import_file_dialog_();
  void open_file_dialog_();
  void save_file_dialog_();

  std::unique_ptr<Occt_view> m_view;

  // Sketch segment manual length input related
  std::function<void(float, bool)> m_dist_callback;
  ScreenCoords                     m_dist_edit_loc {glm::dvec2(0, 0)};
  float                            m_dist_val;

  // Mode related
  Mode                        m_mode         = Mode::Normal;
  Chamfer_mode                m_chamfer_mode = Chamfer_mode::Shape;
  std::vector<Toolbar_button> m_toolbar_buttons;

  // Message status window
  std::string                           m_message;
  bool                                  m_message_visible = false;
  std::chrono::steady_clock::time_point m_message_start_time;

  // Log window
  std::vector<std::string> m_log_messages;                  // Store log messages
  bool                     m_log_scroll_to_bottom = false;  // Flag to auto-scroll to latest message
  bool                     m_log_window_visible   = true;   // Control log window visibility

  // Stream redirection
  std::streambuf* m_original_cout_buf = nullptr;  // Original stdout buffer
  std::streambuf* m_original_cerr_buf = nullptr;  // Original stderr buffer
  LogStreamBuf*   m_cout_log_buf      = nullptr;  // Custom stdout buffer
  LogStreamBuf*   m_cerr_log_buf      = nullptr;  // Custom stderr buffer

  std::string m_last_saved_path;  // Added to store last saved file path
  bool        m_show_sketch_list {true};
  bool        m_hide_all_shapes {false};
  bool        m_show_tool_tips {true};
};