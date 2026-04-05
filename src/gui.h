#pragma once

#include <cstdint>
#include <chrono>  // For message status window (from previous request)
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>  // Added for log messages
#include <utility>
#include <variant>
#include <vector>  // Added for log storage

#include "imgui.h"
#include "log.h"
#include "modes.h"
#include "types.h"

class Lua_console;
class Python_console;
class Occt_view;
class Sketch;
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

  void init(GLFWwindow* window, ImFont* console_font);

  /// Monospace font for script console (normally set via init() from main after io.Fonts load).
  void    set_console_font(ImFont* font) { m_console_font = font; }
  ImFont* console_font() const;

  void render_gui();
  void render_occt();

  void on_key(int key, int scancode, int action, int mods);  // gui_mode.cpp
  void on_mouse_pos(const ScreenCoords& screen_coords);
  void on_mouse_button(int button, int action, int mods);
  void on_mouse_scroll(double xoffset, double yoffset);
  void on_resize(int width, int height);

  // clang-format off
  Mode get_mode() const { return m_mode; }
  Chamfer_mode get_chamfer_mode() const { return m_chamfer_mode; }
  Fillet_mode get_fillet_mode() const { return m_fillet_mode; }
  /// Edge dimension value placement (Options panel, toggle-dimension tool): 0 first point, 1 second, 2 center, 3 auto.
  int edge_dim_label_h() const { return m_edge_dim_label_h; }
  bool get_hide_all_shapes() const { return m_hide_all_shapes; }
  void set_hide_all_shapes(bool hide) { m_hide_all_shapes = hide; }
  bool get_dark_mode() const { return m_dark_mode; }
  ImVec4 get_clear_color() const;
  void set_mode(Mode mode);       // gui_mode.cpp
  void set_parent_mode();        // gui_mode.cpp
  void set_dist_edit(float dist, std::function<void(float, bool)>&& callback, const std::optional<ScreenCoords> screen_coords = std::nullopt);
  void hide_dist_edit();
  void set_angle_edit(float angle, std::function<void(float, bool)>&& callback, const std::optional<ScreenCoords> screen_coords = std::nullopt);
  void hide_angle_edit();
  /// True when dist or angle edit is visible; Tab should be routed to on_key() instead of ImGui.
  bool is_dist_or_angle_edit_active() const;
  void show_message(const std::string& message);
  void log_message(const std::string& message);
  void set_show_options(bool v) { m_show_options = v; }
  void set_show_sketch_list(bool v) { m_show_sketch_list = v; }
  void set_show_shape_list(bool v) { m_show_shape_list = v; }
  void set_log_window_visible(bool v) { m_log_window_visible = v; }
  void set_show_settings_dialog(bool v) { m_show_settings_dialog = v; }
#ifndef NDEBUG
  void set_show_dbg(bool v) { m_show_dbg = v; }
#endif
  // clang-format on

#ifdef __EMSCRIPTEN__
  void open_file_dialog_async();  // Emscripten: hidden <input type="file">; no custom title (browser UI)
  void import_file_dialog_async();                 // STEP / PLY import (routes to on_import_file)
  void save_file_dialog_async(const char* title, const std::string& default_file, const std::string& json_str);
  void download_blob_async(const std::string& default_filename, const std::string& data);
#endif

  void on_file(const std::string& file_path, const std::string& json_str, bool announce_load = true);
  void on_import_file(const std::string& file_path, const std::string& file_data);
  /// Emscripten `on_sketch_underlay_selected` routes here (must be public for C callback).
  void on_sketch_underlay_file(const std::string& file_path, const std::string& file_bytes);

  void save_occt_view_settings();

  /// Default RGB (0–255) for sketch underlay line tint when importing a new image (see Settings).
  void underlay_highlight_color_rgb(uint8_t& r, uint8_t& g, uint8_t& b) const;

  /// For scripting (Lua console): access the 3D view.
  Occt_view* get_view()
  {
    return m_view.get();
  }

 private:
  friend class GUI_access;

  // Structure to hold button state and texture
  struct Toolbar_button
  {
    uint32_t                    texture_id;
    bool                        is_active;
    const char*                 tooltip;
    std::variant<Mode, Command> data;
  };

  void dist_edit_();
  void angle_edit_();
  void sketch_list_();
  void sketch_properties_dialog_();
  void shape_list_();

  // Mode + Options panel (gui_mode.cpp)
  void options_();
  void options_normal_mode_();
  void options_move_mode_();
  void options_scale_mode_();
  void on_key_move_mode_(int key);
  void on_key_rotate_mode_(int key);
  void options_sketch_operation_axis_mode_();
  void options_shape_chamfer_mode_();
  void options_shape_fillet_mode_();
  void options_shape_polar_duplicate_mode_();
  void options_rotate_mode_();

  void dbg_();
  void initialize_toolbar_();
  void load_examples_list_();
  void load_default_project_();
  void menu_bar_();
  void toolbar_();
  void message_status_window_();
  void add_box_dialog_();
  void add_pyramid_dialog_();
  void add_sphere_dialog_();
  void add_cylinder_dialog_();
  void add_cone_dialog_();
  void add_torus_dialog_();
  void log_window_();
  void lua_console_();
  void python_console_();
  void settings_();
  void setup_log_redirection_();
  void cleanup_log_redirection_();

  // Import/export related
  void import_file_dialog_();
  void export_file_dialog_(Export_format fmt);

  void sketch_underlay_import_dialog_();
  void sketch_underlay_panel_(const std::shared_ptr<Sketch>& sk);
#if defined(__EMSCRIPTEN__)
  void sketch_underlay_file_dialog_async();
#endif

  // Open/save related
  void open_file_dialog_();
  void save_file_dialog_();

  void        save_startup_project_();
  void        clear_saved_startup_project_();
  /// Native only: store path in settings after a successful Open (for optional startup load).
  void        persist_last_opened_project_path_(const std::string& path);
  std::string serialized_project_json_() const;
  void        open_url_(const char* url);

  // Settings (gui_settings.cpp)
  void load_occt_view_settings_();
  void parse_occt_view_ini_(const std::string& content);
  void parse_occt_view_settings_(const std::string& content);
  void parse_gui_panes_settings_(const std::string& content);
  void apply_imgui_rounding_from_members_();
  void imgui_rounding_fallbacks_from_theme_(float& general, float& scroll, float& tabs) const;

  std::unique_ptr<Occt_view> m_view;

  // Sketch segment manual length input related
  std::function<void(float, bool)> m_dist_callback;
  ScreenCoords                     m_dist_edit_loc {glm::dvec2(0, 0)};
  float                            m_dist_val;

  // Sketch segment manual angle input related
  std::function<void(float, bool)> m_angle_callback;
  ScreenCoords                     m_angle_edit_loc {glm::dvec2(0, 0)};
  float                            m_angle_val;

  // Mode related
  Mode                        m_mode         = Mode::Normal;
  Chamfer_mode                m_chamfer_mode = Chamfer_mode::Shape;
  Fillet_mode                 m_fillet_mode  = Fillet_mode::Shape;
  int                         m_edge_dim_label_h {3};  // Prs3d_DTHP_Fit
  std::vector<Toolbar_button> m_toolbar_buttons;

  // Message status window
  std::string                           m_message;
  bool                                  m_message_visible = false;
  std::chrono::steady_clock::time_point m_message_start_time;

  // Log window (single buffer for ImGui read-only multiline = selectable / copyable text)
  std::vector<char> m_log_buffer {'\0'};
  bool              m_log_scroll_to_bottom = false;  // Auto-scroll log to bottom on new lines (like Lua console)
  bool              m_log_window_visible   = true;   // Control log window visibility

  // Stream redirection
  std::streambuf* m_original_cout_buf = nullptr;  // Original stdout buffer
  std::streambuf* m_original_cerr_buf = nullptr;  // Original stderr buffer
  Log_strm*       m_cout_log_buf      = nullptr;  // Custom stdout buffer
  Log_strm*       m_cerr_log_buf      = nullptr;  // Custom stderr buffer

  std::string                                      m_last_saved_path;  // Session path for Ctrl+S / Save as
  bool                                             m_load_last_opened_on_startup {false};
  std::string                                      m_last_opened_project_path;  // Persisted in settings (native)
  std::vector<std::pair<std::string, std::string>> m_example_files;    // (display_name, path) for Examples menu
  bool                                             m_show_sketch_list {true};
  bool                                             m_show_shape_list {true};
  bool                                             m_show_options {true};
  bool                                             m_show_settings_dialog {false};
  bool                                             m_open_add_box_popup {false};
  double                                           m_add_box_origin_x {0};
  double                                           m_add_box_origin_y {0};
  double                                           m_add_box_origin_z {0};
  double                                           m_add_box_width {1};
  double                                           m_add_box_length {1};
  double                                           m_add_box_height {1};
  bool                                             m_open_add_pyramid_popup {false};
  double                                           m_add_pyramid_origin_x {0}, m_add_pyramid_origin_y {0}, m_add_pyramid_origin_z {0};
  double                                           m_add_pyramid_side {1};
  bool                                             m_open_add_sphere_popup {false};
  double                                           m_add_sphere_origin_x {0}, m_add_sphere_origin_y {0}, m_add_sphere_origin_z {0};
  double                                           m_add_sphere_radius {1};
  bool                                             m_open_add_cylinder_popup {false};
  double                                           m_add_cylinder_origin_x {0}, m_add_cylinder_origin_y {0}, m_add_cylinder_origin_z {0};
  double                                           m_add_cylinder_radius {1}, m_add_cylinder_height {1};
  bool                                             m_open_add_cone_popup {false};
  double                                           m_add_cone_origin_x {0}, m_add_cone_origin_y {0}, m_add_cone_origin_z {0};
  double                                           m_add_cone_R1 {1}, m_add_cone_R2 {0}, m_add_cone_height {1};
  bool                                             m_open_add_torus_popup {false};
  double                                           m_add_torus_origin_x {0}, m_add_torus_origin_y {0}, m_add_torus_origin_z {0};
  double                                           m_add_torus_R1 {1}, m_add_torus_R2 {0.5};
  bool                                             m_hide_all_shapes {false};
  bool                                             m_show_tool_tips {true};
  bool                                             m_dark_mode {false};
#ifndef NDEBUG
  bool m_show_dbg {false};
#endif
  bool                         m_show_lua_console {true};  // Lua Console pane; hidden if false in settings
  /// ImGui corner radii (applied after StyleColorsDark/Light each frame). Scroll value sets both scrollbar and grab rounding.
  float                        m_imgui_rounding_general {0.f};
  float                        m_imgui_rounding_scroll {0.f};
  float                        m_imgui_rounding_tabs {0.f};
  void*                        m_underlay_panel_sketch {nullptr};
  bool                         m_sketch_properties_open {false};
  std::weak_ptr<Sketch>        m_sketch_properties_sketch;
  /// If set, next underlay import (menu or async) applies to this sketch; otherwise current sketch.
  std::weak_ptr<Sketch>        m_underlay_import_sketch_target;
  double                       m_ul_cx {};
  double                       m_ul_cy {};
  double                       m_ul_hw {};
  double                       m_ul_hh {};
  double                       m_ul_rot {};
  float                        m_ul_opacity {0.88f};
  bool                         m_ul_vis {true};
  bool                         m_ul_key_white {true};
  bool                         m_ul_line_tint {true};
  float                        m_ul_tint_col[3] {1.f, 220.f / 255.f, 0.f};
  /// Default underlay tint for new imports (0–1, persisted in ezycad_settings.json).
  float                        m_underlay_highlight_color[3] {1.f, 220.f / 255.f, 0.f};

  std::unique_ptr<Lua_console> m_lua_console;
  bool                         m_show_python_console {false};
  std::unique_ptr<Python_console> m_python_console;
  ImFont*                      m_console_font {nullptr};  // Cousine monospace; set from main
};

/// OCCT standard material display names for ImGui combos (index matches \c Graphic3d_NameOfMaterial).
const std::vector<std::string>& occt_material_combo_labels();