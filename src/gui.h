#pragma once

#include <array>
#include <chrono> // For message status window (from previous request)
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <gp_Pnt2d.hxx>
#include <gp_Vec2d.hxx>
#include <memory>
#include <string> // Added for log messages
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "geom.h"
#include "imgui.h"
#include "imgui_markdown.h"
#include "log.h"
#include "modes.h"
#include "occt_view.h"
#include "types.h"

class Lua_console;
class Python_console;
class Sketch;
struct GLFWwindow;

enum class Command
{
  Shape_cut,
  Shape_fuse,
  Shape_common,
  _count
};

/// Two-segment picks for underlay X/Y calibration (Sketch properties pane).
enum class Underlay_calib_phase : std::uint8_t
{
  None,
  PickX1,
  PickX2,
  AwaitDistX,
  PickY1,
  PickY2,
  AwaitDistY,
  /// Two clicks: bitmap corner (0,0), then a point along bitmap +U (datum / origin on plane).
  PickDatumO,
  PickDatumU
};
/// One entry under File > Examples (menu label + path to `.ezy` on disk).
struct Example_file
{
  std::string label;
  std::string path;
};
/// Default OCCT line-width scale for length dimensions when `edge_dim_line_width` is missing from settings JSON.
inline constexpr float k_gui_edge_dim_line_width_default = 1.0f;
/// Default OCCT arrow length for length dimensions when `edge_dim_arrow_size` is missing from settings JSON.
inline constexpr float k_gui_edge_dim_arrow_size_default = 6.0f;
/// Default dimension line/text RGB when `edge_dim_color` is missing (yellow).
inline constexpr float k_gui_edge_dim_color_default[3] = {1.f, 1.f, 0.f};
/// Text height scale for length dimension labels (`gui.edge_dim_text_scale`).
inline constexpr float k_gui_edge_dim_text_scale_min     = 0.5f;
inline constexpr float k_gui_edge_dim_text_scale_max     = 3.0f;
inline constexpr float k_gui_edge_dim_text_scale_default = 1.0f;
/// Arrow style preset index (`gui.edge_dim_arrow_style`): 0 standard, 1 sharp, 2 wide, 3 3D.
inline constexpr int k_gui_edge_dim_arrow_style_min = 0;
inline constexpr int k_gui_edge_dim_arrow_style_max = 3;
/// Arrow orientation (`gui.edge_dim_arrow_orientation`): 0 fit, 1 internal, 2 external.
inline constexpr int k_gui_edge_dim_arrow_orientation_min = 0;
inline constexpr int k_gui_edge_dim_arrow_orientation_max = 2;
/// `gui.edge_dim_text_render_mode` indices.
inline constexpr int k_gui_edge_dim_text_render_opaque_2d     = 0;
inline constexpr int k_gui_edge_dim_text_render_common_color  = 1;
inline constexpr int k_gui_edge_dim_text_render_2d_screen     = 2;
inline constexpr int k_gui_edge_dim_text_render_3d_text       = 3;
inline constexpr int k_gui_edge_dim_text_render_z_top         = 4;
inline constexpr int k_gui_edge_dim_text_render_z_topmost     = 5;
inline constexpr int k_gui_edge_dim_text_render_mode_default = k_gui_edge_dim_text_render_z_topmost;
inline constexpr int k_gui_edge_dim_text_render_mode_max     = k_gui_edge_dim_text_render_z_topmost;
/// Scale factor for permanent sketch-node '+' annotations (`gui.permanent_node_anno_scale`).
inline constexpr float k_gui_permanent_node_anno_scale_min     = 0.25f;
inline constexpr float k_gui_permanent_node_anno_scale_max     = 3.0f;
inline constexpr float k_gui_permanent_node_anno_scale_default = 1.0f;
/// Allowed range and default for `gui.view_roll_step_deg` (view roll and numpad orbit steps; must match Settings slider).
inline constexpr double k_gui_view_roll_step_deg_min     = 0.1;
inline constexpr double k_gui_view_roll_step_deg_max     = 180.0;
inline constexpr double k_gui_view_roll_step_deg_default = 45.0;
/// Allowed range and default for `gui.view_zoom_scroll_scale` (wheel/keyboard zoom units; must match Settings slider).
inline constexpr double k_gui_view_zoom_scroll_scale_min     = 0.25;
inline constexpr double k_gui_view_zoom_scroll_scale_max     = 64.0;
inline constexpr double k_gui_view_zoom_scroll_scale_default = 4.0;
/// `gui.ui_verbosity`: 0 = minimal UI; odd steps unlock feature tiers; even steps unlock help tiers.
inline constexpr int k_gui_ui_verbosity_min            = 0;
inline constexpr int k_gui_ui_verbosity_default        = 6;
inline constexpr int k_gui_ui_feature_tier_max         = 3;
inline constexpr int k_gui_ui_help_tier_max            = 3;
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

  void         on_key(int key, int scancode, int action, int mods); // gui_mode.cpp
  void         on_mouse_pos(const ScreenCoords& screen_coords);
  void         on_mouse_button(int button, int action, int mods);
  void         on_mouse_scroll(double xoffset, double yoffset);
  void         on_resize(int width, int height);
  Mode         get_mode() const { return m_mode; }
  Chamfer_mode get_chamfer_mode() const { return m_chamfer_mode; }
  Fillet_mode  get_fillet_mode() const { return m_fillet_mode; }
  /// Edge dimension value placement: 0 first point, 1 second, 2 center, 3 auto.
  int edge_dim_label_h() const { return m_edge_dim_label_h; }
  /// OCCT scale factor for sketch/extrude length dimension lines (1.0 = default thickness).
  float edge_dim_line_width() const { return m_edge_dim_line_width; }
  /// OCCT arrow length for sketch/extrude length dimensions.
  float edge_dim_arrow_size() const { return m_edge_dim_arrow_size; }
  /// When false, length dimensions are hidden on all sketches until re-enabled.
  bool show_sketch_dimensions() const { return m_show_sketch_dimensions; }
  void set_show_sketch_dimensions(bool show);
  /// Bundle of global length-dimension display settings for OCCT annotations.
  [[nodiscard]] Length_dimension_style length_dimension_style() const;
  /// Reapply dimension visibility on all sketches (global show flag + current tool mode).
  void apply_sketch_dimensions_visibility();
  /// Scale factor for permanent sketch-node '+' annotations.
  float  permanent_node_anno_scale() const { return m_permanent_node_anno_scale; }
  bool   get_hide_all_shapes() const { return m_hide_all_shapes; }
  void   set_hide_all_shapes(bool hide) { m_hide_all_shapes = hide; }
  /// Orthographic camera in Inspection mode (Mode::Normal); persisted as `gui.inspection_orthographic`.
  bool   inspection_orthographic() const { return m_inspection_orthographic; }
  void   set_inspection_orthographic(bool v) { m_inspection_orthographic = v; }
  bool   get_dark_mode() const { return m_dark_mode; }
  ImVec4 get_clear_color() const;
  void   set_mode(Mode mode); // gui_mode.cpp
  void   set_parent_mode();   // gui_mode.cpp
  void   set_dist_edit(float dist, std::function<void(float, bool)>&& callback,
                       const std::optional<ScreenCoords> screen_coords = std::nullopt);
  void   hide_dist_edit();
  void   set_angle_edit(float angle, std::function<void(float, bool)>&& callback,
                        const std::optional<ScreenCoords> screen_coords = std::nullopt);
  void   hide_angle_edit();
  /// True when dist or angle edit is visible; Tab should be routed to on_key() instead of ImGui.
  bool is_dist_or_angle_edit_active() const;
  void show_message(const std::string& message);
  void log_message(const std::string& message);
  void set_show_options(bool v) { m_show_options = v; }
  void set_show_sketch_list(bool v) { m_show_sketch_list = v; }
  void set_show_shape_list(bool v) { m_show_shape_list = v; }
  void set_log_window_visible(bool v) { m_log_window_visible = v; }
  void set_show_settings_dialog(bool v) { m_show_settings_dialog = v; }
  int  ui_verbosity() const { return m_ui_verbosity; }
  void set_ui_verbosity(int v);
  /// Cumulative feature depth from `gui.ui_verbosity` (F1 at verbosity >= 1).
  int ui_feature_tier() const { return (m_ui_verbosity + 1) / 2; }
  /// Cumulative help depth from `gui.ui_verbosity` (H1 at verbosity >= 2).
  int ui_help_tier() const { return m_ui_verbosity / 2; }
  bool ui_show_feature(int tier) const { return tier <= ui_feature_tier(); }
  bool ui_show_help(int tier) const { return tier <= ui_help_tier(); }
  /// Sketch list: reserved [P] column at verbosity >= 2; active button at feature tier 2 (verbosity >= 3).
  bool ui_show_sketch_list_props_slot() const { return m_ui_verbosity >= 2; }
  bool ui_show_sketch_list_props_button() const { return ui_show_feature(2); }
  bool ui_show_sketch_list_expand() const { return ui_show_feature(2); }
  bool show_options_effective() const { return m_show_options && ui_show_feature(1); }
  bool show_sketch_list_effective() const { return m_show_sketch_list && ui_show_feature(1); }
  bool show_shape_list_effective() const { return m_show_shape_list && ui_show_feature(1); }
  bool log_window_visible_effective() const { return m_log_window_visible && ui_show_feature(1); }
  bool show_lua_console_effective() const { return m_show_lua_console && ui_show_feature(2); }
  bool show_python_console_effective() const { return m_show_python_console && ui_show_feature(2); }
#ifndef NDEBUG
  void set_show_dbg(bool v) { m_show_dbg = v; }
#endif

#ifdef __EMSCRIPTEN__
  void open_file_dialog_async();   // Emscripten: hidden <input type="file">; no custom title (browser UI)
  void import_file_dialog_async(); // STEP / PLY import (routes to on_import_file)
  void save_file_dialog_async(const char* title, const std::string& default_file, const std::string& json_str);
  void download_blob_async(const std::string& default_filename, const std::string& data);
  /// After browser download save, remember basename for window title and Save-as default.
  void note_saved_project_filename(const std::string& filename);
#endif

  void on_file(const std::string& file_path, const std::string& json_str, bool announce_load = true);
  void on_import_file(const std::string& file_path, const std::string& file_data);
  /// Emscripten `on_sketch_underlay_selected` routes here (must be public for C callback).
  void on_sketch_underlay_file(const std::string& file_path, const std::string& file_bytes);

  void save_occt_view_settings();

  /// JSON for scripting: `occt_view` (background, grid) plus sketch dimension keys under `gui.*` (see `ezycad_settings.json`).
  /// Asserts if the OCCT view is missing.
  [[nodiscard]] std::string occt_view_settings_json() const;

  /// Default RGBA (0-255) for sketch underlay line tint when importing a new image (see Settings).
  void underlay_highlight_color_rgba(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const;
  /// RGBA (0-255) for Shape List row hover highlight in the 3D viewer (see Settings).
  void shape_list_hover_color_rgba(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const;
  /// For scripting (Lua console): access the 3D view.
  Occt_view* get_view() { return m_view.get(); }

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
  void sketch_list_inspector_(Sketch& sketch, int index);
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

  void         dbg_();
  void         initialize_toolbar_();
  void         load_examples_list_();
  void         load_default_project_();
  void         menu_bar_();
  void         toolbar_();
  void         message_status_window_();
  void         add_box_dialog_();
  void         add_pyramid_dialog_();
  void         add_sphere_dialog_();
  void         add_cylinder_dialog_();
  void         add_cone_dialog_();
  void         add_torus_dialog_();
  void         add_sketch_dialog_();
  void         about_dialog_();
  void         ensure_about_assets_();
  static float list_name_field_width_(const ImGuiStyle& st, float max_name_text_w);
  void         log_window_();
  void         lua_console_();
  void         python_console_();
  void         settings_();
  void         setup_log_redirection_();
  void         cleanup_log_redirection_();

  // About dialog markdown callbacks (for links and images in `about.md`).
  ImGui::MarkdownImageData        about_markdown_image_(ImGui::MarkdownLinkCallbackData data);
  static void                     about_markdown_link_cb_(ImGui::MarkdownLinkCallbackData data);
  static ImGui::MarkdownImageData about_markdown_image_cb_(ImGui::MarkdownLinkCallbackData data);

  // Import/export related
  void import_file_dialog_();
  void export_file_dialog_(Export_format fmt);

  // Sketch underlay related
  void sketch_underlay_import_dialog_();
  void sketch_underlay_panel_settings_(const std::shared_ptr<Sketch>& sk);
  void cancel_underlay_calib_();
  bool try_underlay_calib_click_(const ScreenCoords& screen_coords);
  void begin_underlay_calib_set_x_(const std::shared_ptr<Sketch>& sk);
  void begin_underlay_calib_set_y_(const std::shared_ptr<Sketch>& sk);
  void begin_underlay_calib_define_datum_(const std::shared_ptr<Sketch>& sk);
  void underlay_calib_prompt_x_distance_(const std::shared_ptr<Sketch>& sk);
  void underlay_calib_prompt_y_distance_(const std::shared_ptr<Sketch>& sk);
#if defined(__EMSCRIPTEN__)
  void sketch_underlay_file_dialog_async();
#endif

  // Open/save related
  void new_project_();
  void open_file_dialog_();
  void save_file_dialog_();

  void save_startup_project_();
  void clear_saved_startup_project_();
  /// Native only: store path in settings after a successful Open (for optional startup load).
  void                      persist_last_opened_project_path_(const std::string& path);
  [[nodiscard]] std::string serialized_project_json_() const;
  void                      open_url_(const std::string& url);
  void                      update_window_title_();
  [[nodiscard]] std::string project_title_segment_() const;
  /// Parses a float from manual dist/angle ImGui text fields (trimmed, full-string match).
  [[nodiscard]] static bool parse_dist_text_to_float_(const char* buf, float& out);
  /// True if JSON parses and looks like an EzyCad project document (`sketches` array[).
  [[nodiscard]] static bool is_valid_project_json_(const std::string& s);

  /// OCCT standard material display names for ImGui combos (index matches \c Graphic3d_NameOfMaterial).
  [[nodiscard]] static const std::vector<std::string>& occt_material_combo_labels_();

  // Settings (gui_settings.cpp)
  void load_occt_view_settings_();
  void parse_occt_view_settings_(const std::string& content);
  void parse_gui_panes_settings_(const std::string& content);
  void apply_imgui_rounding_from_members_();
  void imgui_rounding_fallbacks_from_theme_(float& general, float& scroll, float& tabs) const;

  Occt_view::uptr m_view;
  GLFWwindow*     m_glfw_window{nullptr};
  std::string     m_cached_window_title;

  // Sketch segment manual length input related
  std::function<void(float, bool)> m_dist_callback;
  ScreenCoords                     m_dist_edit_loc{glm::dvec2(0, 0)};
  float                            m_dist_val{};
  std::array<char, 64>             m_dist_text_buf{};
  bool                             m_dist_edit_focus_pending{false};

  // Sketch segment manual angle input related
  std::function<void(float, bool)> m_angle_callback;
  ScreenCoords                     m_angle_edit_loc{glm::dvec2(0, 0)};
  float                            m_angle_val{};
  std::array<char, 64>             m_angle_text_buf{};
  bool                             m_angle_edit_focus_pending{false};

  // Mode related
  Mode         m_mode                = Mode::Normal;
  Chamfer_mode m_chamfer_mode        = Chamfer_mode::Shape;
  Fillet_mode  m_fillet_mode         = Fillet_mode::Shape;
  int   m_edge_dim_label_h              = 3;
  float m_edge_dim_line_width           = k_gui_edge_dim_line_width_default;
  float m_edge_dim_arrow_size           = k_gui_edge_dim_arrow_size_default;
  float m_edge_dim_color[3]             = {k_gui_edge_dim_color_default[0], k_gui_edge_dim_color_default[1],
                             k_gui_edge_dim_color_default[2]};
  float m_edge_dim_text_scale           = k_gui_edge_dim_text_scale_default;
  int   m_edge_dim_arrow_style          = 0;
  int   m_edge_dim_arrow_orientation    = 0;
  int   m_edge_dim_text_render_mode     = k_gui_edge_dim_text_render_mode_default;
  bool  m_show_sketch_dimensions        = true;
  float        m_permanent_node_anno_scale = k_gui_permanent_node_anno_scale_default;
  /// Degrees per numpad orbit (8/2/4/6) and Blender-style roll (Shift+NumPad 4/6); persisted in `gui.view_roll_step_deg`.
  double m_view_roll_step_deg = k_gui_view_roll_step_deg_default;
  /// Multiplier for `UpdateZoom(Aspect_ScrollDelta(..., int(y * scale)))`; persisted in `gui.view_zoom_scroll_scale`.
  double                      m_view_zoom_scroll_scale = k_gui_view_zoom_scroll_scale_default;
  bool                        m_inspection_orthographic  = false;
  std::vector<Toolbar_button> m_toolbar_buttons;

  // Message status window
  std::string                           m_message;
  bool                                  m_message_visible = false;
  std::chrono::steady_clock::time_point m_message_start_time;

  // Log window (single buffer for ImGui read-only multiline = selectable / copyable text)
  std::vector<char> m_log_buffer{'\0'};
  bool              m_log_scroll_to_bottom = false; // Auto-scroll log to bottom on new lines (like Lua console)
  bool              m_log_window_visible   = true;  // Control log window visibility

  // Stream redirection
  std::streambuf* m_original_cout_buf = nullptr; // Original stdout buffer
  std::streambuf* m_original_cerr_buf = nullptr; // Original stderr buffer
  Log_strm*       m_cout_log_buf      = nullptr; // Custom stdout buffer
  Log_strm*       m_cerr_log_buf      = nullptr; // Custom stderr buffer

  std::string m_last_saved_path; // Session path for Ctrl+S / Save as
  bool        m_load_last_opened_on_startup{false};
  std::string m_last_opened_project_path; // Persisted in settings (native)

  using Example_file_list = std::vector<Example_file>;
  Example_file_list m_example_files;

  std::unordered_map<const Sketch*, bool> m_sketch_list_expanded;

  bool        m_show_sketch_list{true};
  bool        m_show_shape_list{true};
  bool        m_show_options{true};
  bool        m_show_settings_dialog{false};
  bool        m_open_about_popup{false};
  bool        m_about_popup_open{false};
  std::string m_about_markdown;
  uint32_t    m_about_splash_gl{0};
  glm::ivec2  m_about_splash_size{512, 512};
  bool        m_about_assets_loaded{false};
  bool        m_open_add_box_popup{false};
  glm::dvec3  m_add_box_origin{0.0, 0.0, 0.0};
  glm::dvec3  m_add_box_size{1.0, 1.0, 1.0};
  bool        m_open_add_pyramid_popup{false};
  glm::dvec3  m_add_pyramid_origin{0.0, 0.0, 0.0};
  double      m_add_pyramid_side{1};
  bool        m_open_add_sphere_popup{false};
  glm::dvec3  m_add_sphere_origin{0.0, 0.0, 0.0};
  double      m_add_sphere_radius{1};
  bool        m_open_add_cylinder_popup{false};
  glm::dvec3  m_add_cylinder_origin{0.0, 0.0, 0.0};
  double      m_add_cylinder_radius{1}, m_add_cylinder_height{1};
  bool        m_open_add_cone_popup{false};
  glm::dvec3  m_add_cone_origin{0.0, 0.0, 0.0};
  double      m_add_cone_R1{1}, m_add_cone_R2{0}, m_add_cone_height{1};
  bool        m_open_add_torus_popup{false};
  glm::dvec3  m_add_torus_origin{0.0, 0.0, 0.0};
  double      m_add_torus_R1{1}, m_add_torus_R2{0.5};
  bool        m_open_add_sketch_popup{false};
  int         m_new_sketch_plane{0}; // 0=XY, 1=XZ, 2=YZ
  double      m_new_sketch_offset{};
  bool        m_hide_all_shapes{false};
  int         m_ui_verbosity{k_gui_ui_verbosity_default};
  bool        m_dark_mode{false};
#ifndef NDEBUG
  bool m_show_dbg{false};
#endif
  bool m_show_lua_console{true}; // Lua Console pane; hidden if false in settings
  /// ImGui corner radii (applied after StyleColorsDark/Light each frame). Scroll value sets both scrollbar and grab rounding.
  float                 m_imgui_rounding_general{0.f};
  float                 m_imgui_rounding_scroll{0.f};
  float                 m_imgui_rounding_tabs{0.f};
  bool                  m_sketch_properties_open{false};
  std::weak_ptr<Sketch> m_sketch_properties_sketch;

  // Sketch underlay related
  void*                 m_underlay_panel_sketch{nullptr};
  Underlay_calib_phase  m_underlay_calib_phase{Underlay_calib_phase::None};
  std::weak_ptr<Sketch> m_underlay_calib_sketch_wk{};
  gp_Pnt2d              m_underlay_calib_x0{};
  gp_Pnt2d              m_underlay_calib_x1{};
  gp_Vec2d              m_underlay_calib_axis_u{}; // After X distance (model units)
  gp_Pnt2d              m_underlay_calib_y0{};
  gp_Pnt2d              m_underlay_calib_y1{};
  gp_Pnt2d              m_underlay_calib_datum_o{};
  /// If set, next underlay import (menu or async) applies to this sketch; otherwise current sketch.
  std::weak_ptr<Sketch> m_underlay_import_sketch_target;
  glm::dvec2            m_underlay_center{};
  glm::dvec2            m_underlay_half_extents{};
  double                m_underlay_rot{};
  float                 m_underlay_opacity{0.88f};
  bool                  m_underlay_vis{true};
  bool                  m_underlay_key_white{true};
  bool                  m_underlay_line_tint{true};
  glm::vec4             m_underlay_tint_col{1.f, 220.f / 255.f, 0.f, 1.f};
  /// Default underlay tint for new imports (0-1, persisted in ezycad_settings.json).
  glm::vec4 m_underlay_highlight_color{1.f, 220.f / 255.f, 0.f, 1.f};
  /// Shape List hover highlight in the OCCT view (0-1, persisted in ezycad_settings.json).
  glm::vec4 m_shape_list_hover_color{0.f, 1.f, 0.f, 1.f};

  std::unique_ptr<Lua_console>    m_lua_console;
  bool                            m_show_python_console{false};
  std::unique_ptr<Python_console> m_python_console;
  ImFont*                         m_console_font{nullptr}; // Cousine monospace; set from main
};