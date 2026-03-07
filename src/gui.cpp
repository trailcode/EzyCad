#include "gui.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>

#include "settings.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else

#include "third_party/tinyfiledialogs/tinyfiledialogs.h"
#endif

#include "geom.h"
#include "imgui.h"
#include "log.h"
#include "lua_console.h"
#include "occt_view.h"
#include "sketch.h"

// Bump when settings schema or semantics change; mismatch causes defaults to be loaded.
static const char* const k_settings_version = "1";

// Must be here to prevent compiler warning
#include <GLFW/glfw3.h>

#ifndef __EMSCRIPTEN__
#include <cstdlib>
#endif

GUI* gui_instance = nullptr;

GUI::GUI()
{
  m_view = std::make_unique<Occt_view>(*this);
  EZY_ASSERT(!gui_instance);
  gui_instance = this;
}

GUI::~GUI()
{
  cleanup_log_redirection_();  // Clean up stream redirection
}

ImVec4 GUI::get_clear_color() const
{
  if (m_dark_mode)
    return ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
  return ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
}

#ifdef __EMSCRIPTEN__
GUI& GUI::instance()
{
  EZY_ASSERT(gui_instance);
  return *gui_instance;
}
#endif

void GUI::render_gui()
{
  if (m_dark_mode)
    ImGui::StyleColorsDark();
  else
    ImGui::StyleColorsLight();

  menu_bar_();
  toolbar_();
  dist_edit_();
  angle_edit_();
  sketch_list_();
  shape_list_();
  options_();
  message_status_window_();
  add_box_dialog_();
  add_pyramid_dialog_();
  add_sphere_dialog_();
  add_cylinder_dialog_();
  add_cone_dialog_();
  add_torus_dialog_();
  log_window_();
  lua_console_();
  settings_();
  dbg_();
}

void GUI::render_occt()
{
  m_view->do_frame();
}

void GUI::set_mode(Mode mode)
{
  m_mode = mode;
  m_view->on_mode();
  for (Toolbar_button& b : m_toolbar_buttons)
    if (b.data.index() == 0)
      b.is_active = std::get<Mode>(b.data) == mode;
}

void GUI::set_parent_mode()
{
  static std::map<Mode, Mode> parent_modes = {
      {                        Mode::Normal,                 Mode::Normal},
      {                          Mode::Move,                 Mode::Normal},
      {                         Mode::Scale,                 Mode::Normal},
      {                        Mode::Rotate,                 Mode::Normal},
      {        Mode::Sketch_inspection_mode,                 Mode::Normal},
      {       Mode::Sketch_from_planar_face,                 Mode::Normal},
      {           Mode::Sketch_face_extrude,                 Mode::Normal},
      {                 Mode::Shape_chamfer,                 Mode::Normal},
      {                  Mode::Shape_fillet,                 Mode::Normal},
      {         Mode::Shape_polar_duplicate,                 Mode::Normal},
      {               Mode::Sketch_add_node, Mode::Sketch_inspection_mode},
      {               Mode::Sketch_add_edge, Mode::Sketch_inspection_mode},
      {        Mode::Sketch_add_multi_edges, Mode::Sketch_inspection_mode},
      {     Mode::Sketch_add_seg_circle_arc, Mode::Sketch_inspection_mode},
      {         Mode::Sketch_operation_axis, Mode::Sketch_inspection_mode},
      {             Mode::Sketch_add_square, Mode::Sketch_inspection_mode},
      {          Mode::Sketch_add_rectangle, Mode::Sketch_inspection_mode},
      {Mode::Sketch_add_rectangle_center_pt, Mode::Sketch_inspection_mode},
      {             Mode::Sketch_add_circle, Mode::Sketch_inspection_mode},
      {       Mode::Sketch_add_circle_3_pts, Mode::Sketch_inspection_mode},
      {               Mode::Sketch_add_slot, Mode::Sketch_inspection_mode},
      {        Mode::Sketch_toggle_edge_dim, Mode::Sketch_inspection_mode},
  };

  static bool check = [&]()
  {
    // Called only once.
    for (size_t idx = 0; idx < size_t(Mode::_count); ++idx)
      EZY_ASSERT(parent_modes.find(Mode(idx)) != parent_modes.end());

    return true;
  }();

  const auto itr = parent_modes.find(get_mode());
  EZY_ASSERT(itr != parent_modes.end());
  set_mode(itr->second);
}

void GUI::on_key(int key, int scancode, int action, int mods)
{
  if (action == GLFW_PRESS)
  {
    const ScreenCoords screen_coords(glm::dvec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y));

    // Check for Ctrl modifier
    bool ctrl_pressed = (mods & GLFW_MOD_CONTROL) != 0;

    // Handle file menu hotkeys
    if (ctrl_pressed)
    {
      switch (key)
      {
        case GLFW_KEY_N:  // Ctrl+N for New
          m_view->new_file();
          break;

        case GLFW_KEY_O:  // Ctrl+O for Open
          open_file_dialog_();
          break;

        case GLFW_KEY_S:  // Ctrl+S for Save
          save_file_dialog_();
          break;

        default:
          break;
      }
    }
    else
    {
      // Handle other keys
      switch (key)
      {
        case GLFW_KEY_ESCAPE:
          m_view->cancel(Set_parent_mode::Yes);
          hide_dist_edit();
          hide_angle_edit();
          break;

        case GLFW_KEY_TAB:
        {
          bool shift_pressed = (mods & GLFW_MOD_SHIFT) != 0;
          if (shift_pressed)
            m_view->angle_input(screen_coords);
          else
            m_view->dimension_input(screen_coords);
          break;
        }

        case GLFW_KEY_ENTER:
          hide_dist_edit();
          hide_angle_edit();
          m_view->on_enter(screen_coords);
          break;

        case GLFW_KEY_D:
          m_view->delete_selected();
          break;

        case GLFW_KEY_G:
          set_mode(Mode::Move);
          break;

        case GLFW_KEY_R:
          set_mode(Mode::Rotate);
          break;

        case GLFW_KEY_E:
          set_mode(Mode::Sketch_face_extrude);
          break;

        case GLFW_KEY_S:
          set_mode(Mode::Scale);
          break;

        default:
          break;
      }

      switch (get_mode())
      {
        case Mode::Move:
          on_key_move_mode_(key);
          break;

        case Mode::Rotate:
          on_key_rotate_mode_(key);
          break;

        default:
          break;
      }
    }
  }
}

// Initialize toolbar buttons
void GUI::initialize_toolbar_()
{
  m_toolbar_buttons = {
      {                           load_texture("User.png"),  true,                  "Inspection mode",                         Mode::Normal},
      {        load_texture("Workbench_Sketcher_none.png"), false,           "Sketch inspection mode",         Mode::Sketch_inspection_mode},
      {             load_texture("Assembly_AxialMove.png"), false,                   "Shape move (g)",                           Mode::Move},
      {                   load_texture("Draft_Rotate.png"), false,                 "Shape rotate (r)",                         Mode::Rotate},
      {                     load_texture("Part_Scale.png"), false,                      "Shape Scale (s)",                       Mode::Scale},
      {          load_texture("Macro_FaceToSketch_48.png"), false, "Create a sketch from planar face",        Mode::Sketch_from_planar_face},
      {          load_texture("Sketcher_MirrorSketch.png"), false,            "Define operation axis",          Mode::Sketch_operation_axis},
      {           load_texture("Sketcher_CreatePoint.png"), false,                         "Add node",                Mode::Sketch_add_node},
      {     load_texture("Sketcher_Element_Line_Edge.png"), false,                    "Add line edge",                Mode::Sketch_add_edge},
      {                             load_texture("ls.png"), false,              "Add multi-line edge",         Mode::Sketch_add_multi_edges},
      {      load_texture("Sketcher_Element_Arc_Edge.png"), false,                   "Add arc circle",      Mode::Sketch_add_seg_circle_arc},
      {          load_texture("Sketcher_CreateSquare.png"), false,                       "Add square",              Mode::Sketch_add_square},
      {       load_texture("Sketcher_CreateRectangle.png"), false,    "Add rectangle from two points",           Mode::Sketch_add_rectangle},
      {load_texture("Sketcher_CreateRectangle_Center.png"), false,  "Add rectangle with center point", Mode::Sketch_add_rectangle_center_pt},
      {          load_texture("Sketcher_CreateCircle.png"), false,                       "Add circle",              Mode::Sketch_add_circle},
      {    load_texture("Sketcher_Create3PointCircle.png"), false,     "Add circle from three points",        Mode::Sketch_add_circle_3_pts},
      {            load_texture("Sketcher_CreateSlot.png"), false,                         "Add slot",                Mode::Sketch_add_slot},
      {       load_texture("TechDraw_LengthDimension.png"), false, "Toggle edge dimension annotation",         Mode::Sketch_toggle_edge_dim},
      {              load_texture("Design456_Extrude.png"), false,          "Extrude sketch face (e)",            Mode::Sketch_face_extrude},
      {             load_texture("PartDesign_Chamfer.png"), false,                          "Chamfer",                  Mode::Shape_chamfer},
      {              load_texture("PartDesign_Fillet.png"), false,                           "Fillet",                   Mode::Shape_fillet},
      {               load_texture("Draft_PolarArray.png"), false,            "Shape polar duplicate",          Mode::Shape_polar_duplicate},
      {                       load_texture("Part_Cut.png"), false,                        "Shape cut",                   Command::Shape_cut},
      {                      load_texture("Part_Fuse.png"), false,                       "Shape fuse",                  Command::Shape_fuse},
      {                    load_texture("Part_Common.png"), false,                     "Shape common",                Command::Shape_common},
  };
}

void GUI::load_examples_list_()
{
  m_example_files.clear();
#ifdef __EMSCRIPTEN__
  const std::filesystem::path examples_dir("/res/examples");
#else
  const std::filesystem::path examples_dir("res/examples");
#endif
  if (!std::filesystem::is_directory(examples_dir))
    return;

  for (const auto& entry : std::filesystem::directory_iterator(examples_dir))
  {
    if (!entry.is_regular_file())
      continue;

    const auto& p = entry.path();
    if (p.extension() != ".ezy")
      continue;

    std::string path  = p.string();
    std::string label = p.filename().string();
    m_example_files.emplace_back(std::move(label), std::move(path));
  }
  std::sort(m_example_files.begin(), m_example_files.end(),
            [](const auto& a, const auto& b)
            { return a.first < b.first; });
}

void GUI::menu_bar_()
{
  if (!ImGui::BeginMainMenuBar())
    return;

  if (ImGui::BeginMenu("File"))
  {
    if (ImGui::BeginMenu("Examples"))
    {
      for (const auto& [label, path] : m_example_files)
        if (ImGui::MenuItem(label.c_str()))
        {
          std::ifstream file(path);
          std::string   json_str {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
          if (file.good() && !json_str.empty())
            on_file(path, json_str);
          else
            show_message("Error opening example: " + label);
        }

      ImGui::EndMenu();
    }
    if (ImGui::MenuItem("New", "Ctrl+N"))
      m_view->new_file();

    else if (ImGui::MenuItem("Open", "Ctrl+O"))
      open_file_dialog_();

    else if (ImGui::MenuItem("Save", "Ctrl+S"))
      save_file_dialog_();

    else if (ImGui::MenuItem("Save as"))
    {
      m_last_saved_path.clear();  // Force save as dialog
      save_file_dialog_();
    }
    else if (ImGui::MenuItem("Import"))
      import_file_dialog_();

#ifdef __EMSCRIPTEN__
    else if (ImGui::MenuItem("Save settings"))
    {
      save_occt_view_settings();
      show_message("Settings saved");
    }
#endif

    else if (ImGui::MenuItem("Exit"))
      exit(0);

    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Edit"))
  {
    if (ImGui::MenuItem("Add box"))
    {
      const double scale = m_view->get_dimension_scale();
      m_view->add_box(0, 0, 0, scale, scale, scale);
    }
    if (ImGui::MenuItem("Add box_prms"))
    {
      m_add_box_origin_x = 0;
      m_add_box_origin_y = 0;
      m_add_box_origin_z = 0;
      m_add_box_width    = 1.0;
      m_add_box_length   = 1.0;
      m_add_box_height   = 1.0;
      m_open_add_box_popup = true;
    }
    if (ImGui::MenuItem("Add pyramid"))
    {
      const double scale = m_view->get_dimension_scale();
      m_view->add_pyramid(0, 0, 0, scale);
    }
    if (ImGui::MenuItem("Add pyramid_prms"))
    {
      m_add_pyramid_origin_x = m_add_pyramid_origin_y = m_add_pyramid_origin_z = 0;
      m_add_pyramid_side = 1.0;
      m_open_add_pyramid_popup = true;
    }
    if (ImGui::MenuItem("Add sphere"))
    {
      const double scale = m_view->get_dimension_scale();
      m_view->add_sphere(0, 0, 0, scale);
    }
    if (ImGui::MenuItem("Add sphere_prms"))
    {
      m_add_sphere_origin_x = m_add_sphere_origin_y = m_add_sphere_origin_z = 0;
      m_add_sphere_radius = 1.0;
      m_open_add_sphere_popup = true;
    }
    if (ImGui::MenuItem("Add cylinder"))
    {
      const double scale = m_view->get_dimension_scale();
      m_view->add_cylinder(0, 0, 0, scale, scale);
    }
    if (ImGui::MenuItem("Add cylinder_prms"))
    {
      m_add_cylinder_origin_x = m_add_cylinder_origin_y = m_add_cylinder_origin_z = 0;
      m_add_cylinder_radius = m_add_cylinder_height = 1.0;
      m_open_add_cylinder_popup = true;
    }
    if (ImGui::MenuItem("Add cone"))
    {
      const double scale = m_view->get_dimension_scale();
      m_view->add_cone(0, 0, 0, scale, 0.0, scale);
    }
    if (ImGui::MenuItem("Add cone_prms"))
    {
      m_add_cone_origin_x = m_add_cone_origin_y = m_add_cone_origin_z = 0;
      m_add_cone_R1 = 1.0;
      m_add_cone_R2 = 0.0;
      m_add_cone_height = 1.0;
      m_open_add_cone_popup = true;
    }
    if (ImGui::MenuItem("Add torus"))
    {
      const double scale = m_view->get_dimension_scale();
      m_view->add_torus(0, 0, 0, scale, scale / 2.0);
    }
    if (ImGui::MenuItem("Add torus_prms"))
    {
      m_add_torus_origin_x = m_add_torus_origin_y = m_add_torus_origin_z = 0;
      m_add_torus_R1 = 1.0;
      m_add_torus_R2 = 0.5;
      m_open_add_torus_popup = true;
    }

    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("View"))
  {
    bool save_panes = false;
    if (ImGui::MenuItem("Settings", nullptr, m_show_settings_dialog))
    {
      m_show_settings_dialog = !m_show_settings_dialog;
      save_panes             = true;
    }
    if (ImGui::MenuItem("Options", nullptr, m_show_options))
    {
      m_show_options = !m_show_options;
      save_panes     = true;
    }
    if (ImGui::MenuItem("Sketch List", nullptr, m_show_sketch_list))
    {
      m_show_sketch_list = !m_show_sketch_list;
      save_panes         = true;
    }
    if (ImGui::MenuItem("Shape List", nullptr, m_show_shape_list))
    {
      m_show_shape_list = !m_show_shape_list;
      save_panes        = true;
    }
    if (ImGui::MenuItem("Log", nullptr, m_log_window_visible))
    {
      m_log_window_visible = !m_log_window_visible;
      save_panes           = true;
    }
#ifndef __EMSCRIPTEN__
    if (ImGui::MenuItem("Lua Console", nullptr, m_show_lua_console))
    {
      m_show_lua_console = !m_show_lua_console;
      save_panes         = true;
    }
#endif
#ifndef NDEBUG
    if (ImGui::MenuItem("Debug", nullptr, m_show_dbg))
    {
      m_show_dbg = !m_show_dbg;
      save_panes = true;
    }
#endif
    if (save_panes)
      save_occt_view_settings();

    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Help"))
  {
    if (ImGui::MenuItem("About"))
      open_url_("https://github.com/trailcode/EzyCad/blob/main/README.md");

    if (ImGui::MenuItem("Usage Guide"))
      open_url_("https://github.com/trailcode/EzyCad/blob/main/usage.md");

    ImGui::EndMenu();
  }

  ImGui::EndMainMenuBar();
}

void GUI::open_url_(const char* url)
{
#ifdef __EMSCRIPTEN__
  // For Emscripten, use JavaScript's window.open()
  // Use EM_ASM for safer execution
  EM_ASM_({ window.open(UTF8ToString($0), '_blank'); }, url);
#else
  // For native builds, use platform-specific system commands
  std::string cmd;
#ifdef _WIN32
  // Windows: Use 'start' command
  cmd = "start \"\" \"";
  cmd += url;
  cmd += "\"";
#elif defined(__APPLE__)
  // TODO test on macOS
  // macOS: Use 'open' command
  cmd = "open \"";
  cmd += url;
  cmd += "\"";
#else
  // TODO test on Linux
  // Linux and other Unix-like systems: Use 'xdg-open'
  cmd = "xdg-open \"";
  cmd += url;
  cmd += "\"";
#endif
  system(cmd.c_str());
#endif
}

// Settings related
void GUI::parse_occt_view_settings_(const std::string& content)
{
  try
  {
    using namespace nlohmann;
    const json j = json::parse(content);
    if (!j.contains("occt_view") || !j["occt_view"].is_object())
      return;
    const json& ov     = j["occt_view"];
    float       bg1[3] = {0.85f, 0.88f, 0.90f};
    float       bg2[3] = {0.45f, 0.55f, 0.60f};
    int         method = 1;
    float       g1[3]  = {0.1f, 0.1f, 0.1f};
    float       g2[3]  = {0.1f, 0.1f, 0.3f};
    auto        arr3   = [](const json& a, float* out)
    {
      if (a.is_array() && a.size() >= 3)
        for (size_t i = 0; i < 3; ++i)
          if (a[i].is_number())
            out[i] = a[i].get<float>();
    };
    if (ov.contains("bg_color1"))
      arr3(ov["bg_color1"], bg1);
    if (ov.contains("bg_color2"))
      arr3(ov["bg_color2"], bg2);
    if (ov.contains("bg_gradient_method") && ov["bg_gradient_method"].is_number_integer())
      method = ov["bg_gradient_method"].get<int>();
    if (ov.contains("grid_color1"))
      arr3(ov["grid_color1"], g1);
    if (ov.contains("grid_color2"))
      arr3(ov["grid_color2"], g2);
    m_view->set_bg_gradient_colors(bg1[0], bg1[1], bg1[2], bg2[0], bg2[1], bg2[2]);
    m_view->set_bg_gradient_method(method);
    m_view->set_grid_colors(g1[0], g1[1], g1[2], g2[0], g2[1], g2[2]);
  }
  catch (...)
  {
  }
}

void GUI::parse_occt_view_ini_(const std::string& content)
{
  bool               in_section = false;
  std::istringstream ss(content);
  std::string        line;
  float              bg1[3]     = {0.85f, 0.88f, 0.90f};
  float              bg2[3]     = {0.45f, 0.55f, 0.60f};
  int                method     = 1;
  float              g1[3]      = {0.1f, 0.1f, 0.1f};
  float              g2[3]      = {0.1f, 0.1f, 0.3f};
  auto               read_float = [](const std::string& v) -> float
  {
    float              x = 0.f;
    std::istringstream is(v);
    is >> x;
    return x;
  };
  while (std::getline(ss, line))
  {
    if (line.empty())
      continue;
    if (line[0] == '[')
    {
      in_section = (line == "[OCCTView]");
      continue;
    }
    if (!in_section)
      continue;
    size_t eq = line.find('=');
    if (eq == std::string::npos)
      continue;
    std::string key   = line.substr(0, eq);
    std::string value = line.substr(eq + 1);
    if (key == "BgR1")
      bg1[0] = read_float(value);
    else if (key == "BgG1")
      bg1[1] = read_float(value);
    else if (key == "BgB1")
      bg1[2] = read_float(value);
    else if (key == "BgR2")
      bg2[0] = read_float(value);
    else if (key == "BgG2")
      bg2[1] = read_float(value);
    else if (key == "BgB2")
      bg2[2] = read_float(value);
    else if (key == "BgMethod")
    {
      try
      {
        method = std::stoi(value);
      }
      catch (...)
      {
      }
    }
    else if (key == "GridR1")
      g1[0] = read_float(value);
    else if (key == "GridG1")
      g1[1] = read_float(value);
    else if (key == "GridB1")
      g1[2] = read_float(value);
    else if (key == "GridR2")
      g2[0] = read_float(value);
    else if (key == "GridG2")
      g2[1] = read_float(value);
    else if (key == "GridB2")
      g2[2] = read_float(value);
  }
  m_view->set_bg_gradient_colors(bg1[0], bg1[1], bg1[2], bg2[0], bg2[1], bg2[2]);
  m_view->set_bg_gradient_method(method);
  m_view->set_grid_colors(g1[0], g1[1], g1[2], g2[0], g2[1], g2[2]);
}

void GUI::parse_gui_panes_settings_(const std::string& content)
{
  try
  {
    using namespace nlohmann;
    const json j = json::parse(content);
    if (!j.contains("gui") || !j["gui"].is_object())
      return;
    const json& g = j["gui"];
    auto        b = [&g](const char* key, bool current)
    {
      return g.contains(key) && g[key].is_boolean() ? g[key].get<bool>() : current;
    };
    set_show_options(b("show_options", true));
    set_show_sketch_list(b("show_sketch_list", true));
    set_show_shape_list(b("show_shape_list", true));
    set_log_window_visible(b("log_window_visible", true));
    set_show_settings_dialog(b("show_settings_dialog", false));
    m_show_lua_console = b("show_lua_console", false);
#ifndef NDEBUG
    set_show_dbg(b("show_dbg", false));
#endif
  }
  catch (...)
  {
    EZY_ASSERT_MSG(false, "Error parse_gui_panes_settings!");
  }
}

void GUI::load_occt_view_settings_()
{
  std::string content = settings::load_with_defaults();

  try
  {
    using namespace nlohmann;
    const json j          = json::parse(content);
    bool       version_ok = j.contains("version") && j["version"].is_string() &&
                      j["version"].get<std::string>() == k_settings_version;
    if (!version_ok)
    {
      content = settings::load_defaults();
      if (!content.empty())
      {
        try
        {
          json j_default       = json::parse(content);
          j_default["version"] = k_settings_version;
          settings::save(j_default.dump(2));
          content = j_default.dump(2);
        }
        catch (...)
        {
          settings::save(content);
        }
      }
      log_message("Settings version mismatch or missing; loaded defaults.");
    }
  }
  catch (...)
  {
    content = settings::load_defaults();
    if (!content.empty())
      settings::save(content);
  }

#if 0  // Set to 0 to disable logging loaded settings to the log window
  if (!content.empty())
    log_message("Settings loaded:\n" + content);
  else
    log_message("Settings loaded: (none)");
#endif

  EZY_ASSERT_MSG(!content.empty(), "Settings content empty!");

  parse_occt_view_settings_(content);
  parse_gui_panes_settings_(content);

  try
  {
    using namespace nlohmann;
    const json j = json::parse(content);
    if (j.contains("imgui_ini") && j["imgui_ini"].is_string())
    {
      const std::string& ini = j["imgui_ini"].get<std::string>();
      if (!ini.empty())
        ImGui::LoadIniSettingsFromMemory(ini.c_str(), ini.size());
    }
  }
  catch (...)
  {
    // EZY_ASSERT_MSG(false, "Settings invalid!");
  }
}

void GUI::save_occt_view_settings()
{
  // log_message("save_occt_view_settings");
  std::string content = settings::load_with_defaults();
  using namespace nlohmann;
  json j;
  if (!content.empty())
  {
    try
    {
      j = json::parse(content);
    }
    catch (...)
    {
    }
  }
  float bg1[3], bg2[3], g1[3], g2[3];
  m_view->get_bg_gradient_colors(bg1, bg2);
  m_view->get_grid_colors(g1, g2);
  int method     = m_view->get_bg_gradient_method();
  j["occt_view"] = {
      {         "bg_color1", {bg1[0], bg1[1], bg1[2]}},
      {         "bg_color2", {bg2[0], bg2[1], bg2[2]}},
      {"bg_gradient_method",                   method},
      {       "grid_color1",    {g1[0], g1[1], g1[2]}},
      {       "grid_color2",    {g2[0], g2[1], g2[2]}},
  };
  j["gui"] = {
      {        "show_options",         m_show_options},
      {    "show_sketch_list",     m_show_sketch_list},
      {     "show_shape_list",      m_show_shape_list},
      {  "log_window_visible",   m_log_window_visible},
      {"show_settings_dialog", m_show_settings_dialog},
      {   "show_lua_console",   m_show_lua_console},
#ifndef NDEBUG
      {            "show_dbg",             m_show_dbg},
#endif
  };
  j["version"]          = k_settings_version;
  const char* imgui_ini = ImGui::SaveIniSettingsToMemory(nullptr);
  if (imgui_ini && *imgui_ini)
    j["imgui_ini"] = std::string(imgui_ini);

  settings::save(j.dump(2));
}

// Render toolbar with ImGui
void GUI::toolbar_()
{
  ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);

  ImVec2 button_size(32, 32);

  for (int i = 0; i < m_toolbar_buttons.size(); i++)
  {
    ImGui::PushID(i);

    bool was_active = false;
    if (m_toolbar_buttons[i].is_active)
    {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.06f, 0.53f, 0.98f, 1.00f));
      was_active = true;
    }

    // Add a unique string ID (e.g., "button0", "button1", etc.)
    char button_id[16];
    snprintf(button_id, sizeof(button_id), "button%d", i);
    if (ImGui::ImageButton(button_id, (ImTextureID) (intptr_t) m_toolbar_buttons[i].texture_id, button_size))
    {
      if (m_toolbar_buttons[i].data.index() == 1)
        switch (std::get<Command>(m_toolbar_buttons[i].data))
        {
          case Command::Shape_cut:
            if (Status s = m_view->shp_cut().selected_cut(); !s.is_ok())
              show_message(s.message());

            break;

          case Command::Shape_fuse:
            if (Status s = m_view->shp_fuse().selected_fuse(); !s.is_ok())
              show_message(s.message());

            break;

          case Command::Shape_common:
            if (Status s = m_view->shp_common().selected_common(); !s.is_ok())
              show_message(s.message());

            break;

          default:
            EZY_ASSERT(false);
        }
      else
      {
        m_toolbar_buttons[i].is_active = true;
        for (int j = 0; j < m_toolbar_buttons.size(); j++)
          if (i != j)
            m_toolbar_buttons[j].is_active = false;

        m_mode = std::get<Mode>(m_toolbar_buttons[i].data);
        m_view->on_mode();
      }
    }

    if (m_show_tool_tips && ImGui::IsItemHovered())
      ImGui::SetTooltip("%s", m_toolbar_buttons[i].tooltip);

    if (was_active)
      ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::PopID();
  }

  ImGui::End();
}

// Distance edit related
void GUI::set_dist_edit(float dist, std::function<void(float, bool)>&& callback, const std::optional<ScreenCoords> screen_coords)
{
  DBG_MSG("dist " << dist);
  m_dist_val = dist;
  if (screen_coords.has_value())
    m_dist_edit_loc = *screen_coords;
  else
    m_dist_edit_loc = ScreenCoords(glm::dvec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y));

  m_dist_callback = std::move(callback);
}

void GUI::hide_dist_edit()
{
  if (m_dist_callback)
  {
    std::function<void(float, bool)> callback;
    // In case the callback sets a new m_dist_callback
    std::swap(callback, m_dist_callback);
    // In case just enter was pressed, or the callback needs to finalize something
    callback(m_dist_val, true);
  }
}

void GUI::dist_edit_()
{
  if (!m_dist_callback)
    return;

  //  Set the position of the next window
  ImGui::SetNextWindowPos(ImVec2(float(m_dist_edit_loc.unsafe_get_x()), float(m_dist_edit_loc.unsafe_get_y())), ImGuiCond_Always);

  // Set a small size (optional)
  ImGui::SetNextWindowSize(ImVec2(80.0f, 25.0f), ImGuiCond_Once);

  // Begin a window with minimal flags
  ImGui::Begin("FloatEdit##unique_id", nullptr,
               ImGuiWindowFlags_NoTitleBar |
                   ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoSavedSettings);

  ImGui::SetNextItemWidth(80.0f);
  ImGui::SetKeyboardFocusHere();

  // Add a small input float widget and check for changes
  if (ImGui::InputFloat("##dist_edit_float_value", &m_dist_val, 0.0f, 0.0f, "%.2f"))
    m_dist_callback(m_dist_val, false);
  else
    m_dist_val = std::round(m_dist_val * 100.0f) / 100.0f;

  ImGui::End();
}

// Angle edit related
void GUI::set_angle_edit(float angle, std::function<void(float, bool)>&& callback, const std::optional<ScreenCoords> screen_coords)
{
  DBG_MSG("angle " << angle);
  // Only update the value if the input isn't already active (to avoid overwriting user input)
  if (!m_angle_callback)
    m_angle_val = angle;

  if (screen_coords.has_value())
    m_angle_edit_loc = *screen_coords;
  else
    m_angle_edit_loc = ScreenCoords(glm::dvec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y));

  m_angle_callback = std::move(callback);
}

void GUI::hide_angle_edit()
{
  if (m_angle_callback)
  {
    std::function<void(float, bool)> callback;
    // In case the callback sets a new m_angle_callback
    std::swap(callback, m_angle_callback);
    // In case just enter was pressed, or the callback needs to finalize something
    callback(m_angle_val, true);
  }
}

void GUI::angle_edit_()
{
  if (!m_angle_callback)
    return;

  //  Set the position of the next window
  ImGui::SetNextWindowPos(ImVec2(float(m_angle_edit_loc.unsafe_get_x()), float(m_angle_edit_loc.unsafe_get_y())), ImGuiCond_Always);

  // Set a small size (optional)
  ImGui::SetNextWindowSize(ImVec2(80.0f, 25.0f), ImGuiCond_Once);

  // Begin a window with minimal flags
  ImGui::Begin("AngleEdit##unique_id", nullptr,
               ImGuiWindowFlags_NoTitleBar |
                   ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoSavedSettings);

  ImGui::SetNextItemWidth(80.0f);
  ImGui::SetKeyboardFocusHere();

  // Add a small input float widget and check for changes
  if (ImGui::InputFloat("##angle_edit_float_value", &m_angle_val, 0.0f, 0.0f, "%.2f"))
    m_angle_callback(m_angle_val, false);

  ImGui::End();
}

void GUI::sketch_list_()
{
  if (!m_show_sketch_list)
    return;

  if (!ImGui::Begin("Sketch List", &m_show_sketch_list, ImGuiWindowFlags_None))
  {
    ImGui::End();
    return;
  }

  int                     index = 0;
  std::shared_ptr<Sketch> sketch_to_delete;
  for (std::shared_ptr<Sketch>& sketch : m_view->get_sketches())
  {
    EZY_ASSERT(sketch);

    // Buffer for editable name
#pragma warning(push)
#pragma warning(disable : 4996)
    char name_buffer[1024];
    strncpy(name_buffer, sketch->get_name().c_str(), sizeof(name_buffer) - 1);
    name_buffer[sizeof(name_buffer) - 1] = '\0';  // Ensure null-terminated
#pragma warning(pop)

    // Unique ID suffix using index
    std::string id_suffix = "##" + std::to_string(index);

    // Radio button for selection
    ImGui::PushID(("select" + id_suffix).c_str());
    if (ImGui::RadioButton("", &m_view->curr_sketch() == sketch.get()))
      m_view->set_curr_sketch(sketch);

    if (m_show_tool_tips && ImGui::IsItemHovered())
      ImGui::SetTooltip("Sets current");

    ImGui::PopID();

    // Text edit for name
    ImGui::SameLine();
    ImGui::PushID(("name" + id_suffix).c_str());
    if (ImGui::InputText("", name_buffer, sizeof(name_buffer)))
    {
      sketch->set_name(std::string(name_buffer));
      std::cout << "Sketch " << index << " name changed to: " << sketch->get_name() << std::endl;
    }
    // This will open a popup when you right-click on the InputText
    if (ImGui::BeginPopupContextItem("Sketch_InputTextContextMenu"))
    {
      if (ImGui::MenuItem("Delete"))
        sketch_to_delete = sketch;

      ImGui::EndPopup();
    }
    ImGui::PopID();

    // Checkbox for visibility, same line
    ImGui::SameLine();
    ImGui::PushID(("visible" + id_suffix).c_str());
    bool visible = sketch->is_visible();
    if (ImGui::Checkbox("", &visible))
      sketch->set_visible(visible);

    if (m_show_tool_tips && ImGui::IsItemHovered())
      ImGui::SetTooltip("Visibility");

    ++index;
    ImGui::PopID();
  }

  if (sketch_to_delete)
    m_view->remove_sketch(sketch_to_delete);

  ImGui::End();
}

void GUI::shape_list_()
{
  if (!m_show_shape_list)
    return;

  if (!ImGui::Begin("Shape List", &m_show_shape_list, ImGuiWindowFlags_None))
  {
    ImGui::End();
    return;
  }

  int index = 0;

  // Add checkbox for hiding all shapes except current sketches
  if (ImGui::Checkbox("Hide all", &m_hide_all_shapes))
  {
    // Update visibility of all shapes based on the new state
    for (const ShapeBase_ptr& shape : m_view->get_shapes())
    {
      shape->set_visible(!m_hide_all_shapes);
    }
  }

  ImGui::Separator();

  for (const ShapeBase_ptr& shape : m_view->get_shapes())
  {
    // Unique ID suffix using index
    std::string id_suffix = "##" + std::to_string(index++);
    // Editable text box for name
#pragma warning(push)
#pragma warning(disable : 4996)
    char name_buffer[1024];
    strncpy(name_buffer, shape->get_name().c_str(), sizeof(name_buffer) - 1);
    name_buffer[sizeof(name_buffer) - 1] = '\0';  // Ensure null-terminated
#pragma warning(pop)
    ImGui::PushID(("name" + id_suffix).c_str());
    if (ImGui::InputText("", name_buffer, sizeof(name_buffer)))
      shape->set_name(std::string(name_buffer));

    ImGui::PopID();
    // Visibility checkbox
    ImGui::SameLine();
    ImGui::PushID(("visible" + id_suffix).c_str());
    bool visible = shape->get_visible();
    if (ImGui::Checkbox("", &visible))
      shape->set_visible(visible);

    if (m_show_tool_tips && ImGui::IsItemHovered())
      ImGui::SetTooltip("visibility");

    ImGui::PopID();

    // Shaded display mode checkbox
    ImGui::SameLine();
    ImGui::PushID(("shaded" + id_suffix).c_str());
    bool shaded = shape->get_disp_mode() == AIS_Shaded;
    if (ImGui::Checkbox("", &shaded))
      shape->set_disp_mode(shaded ? AIS_Shaded : AIS_WireFrame);

    if (m_show_tool_tips && ImGui::IsItemHovered())
      ImGui::SetTooltip("solid/wire");

    ImGui::PopID();
  }
  ImGui::End();
}

void GUI::options_()
{
  if (!m_show_options)
    return;

  if (!ImGui::Begin("Options", &m_show_options))
  {
    // Pane was collapsed, so skip rendering options to save resources
    ImGui::End();
    return;
  }

  constexpr std::array<std::string_view, 26> c_material_names = {
      "Brass",
      "Bronze",
      "Copper",
      "Gold",
      "Pewter",
      "Plastered",
      "Plastified",
      "Silver",
      "Steel",
      "Stone",
      "Shiny plastified",
      "Satin",
      "Metalized",
      "Ionized",
      "Chrome",
      "Aluminum",
      "Obsidian",
      "Neon",
      "Jade",
      "Charcoal",
      "Water",
      "Glass",
      "Diamond",
      "Transparent",
      "Default",
      "User defined"};

  static std::vector<std::string> material_names;
  if (material_names.empty())
    for (int i = 0; i < Graphic3d_MaterialAspect::NumberOfMaterials(); ++i)
    {
      Graphic3d_MaterialAspect mat(static_cast<Graphic3d_NameOfMaterial>(i));
      material_names.push_back(mat.MaterialName());
    }

  int current_item = int(m_view->get_default_material().Name());
  if (ImGui::BeginCombo("Default Material##filter", material_names[current_item].data(),
                        ImGuiComboFlags_WidthFitPreview | ImGuiComboFlags_HeightSmall))
  // ImGuiComboFlags_HeightSmall))
  {
    for (int i = 0; i < static_cast<int>(material_names.size()); i++)
      if (ImGui::Selectable(material_names[i].data(), current_item == i))
      {
        Graphic3d_MaterialAspect mat(static_cast<Graphic3d_NameOfMaterial>(i));
        m_view->set_default_material(mat);
      }

    ImGui::EndCombo();
  }

  // clang-format off
  switch (get_mode())
  {
    case Mode::Normal:                options_normal_mode_();                 break;
    case Mode::Move:                  options_move_mode_();                   break;
    case Mode::Rotate:                options_rotate_mode_();                 break;
    case Mode::Scale:                 options_scale_mode_();                  break;
    case Mode::Sketch_operation_axis: options_sketch_operation_axis_mode_();  break;
    case Mode::Shape_chamfer:         options_shape_chamfer_mode_();          break;
    case Mode::Shape_fillet:          options_shape_fillet_mode_();           break;
    case Mode::Shape_polar_duplicate: options_shape_polar_duplicate_mode_();  break;
    default:
      break;
  }
  // clang-format on

  if (is_sketch_mode(get_mode()))
  {
    float snap_dist = float(Sketch_nodes::get_snap_dist());
    if (ImGui::InputFloat("Snap dist##float_value", &snap_dist, 1.0f, 2.0f, "%.2f"))
      Sketch_nodes::set_snap_dist(snap_dist);
  }

  ImGui::End();
}

void GUI::settings_()
{
  if (!m_show_settings_dialog)
    return;

  ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_FirstUseEver);  // Auto height
  if (!ImGui::Begin("Settings", &m_show_settings_dialog, ImGuiWindowFlags_None))
  {
    ImGui::End();
    save_occt_view_settings();  // Persist that dialog was closed (e.g. via X)
    return;
  }

  if (ImGui::Checkbox("Dark mode", &m_dark_mode))
    save_occt_view_settings();

  if (ImGui::CollapsingHeader("3D view background"))
  {
    float bg1[3], bg2[3];
    m_view->get_bg_gradient_colors(bg1, bg2);
    bool bg_changed = false;
    if (ImGui::ColorEdit3("Background color 1", bg1, ImGuiColorEditFlags_Float))
      bg_changed = true;
    if (ImGui::ColorEdit3("Background color 2", bg2, ImGuiColorEditFlags_Float))
      bg_changed = true;
    if (bg_changed)
    {
      m_view->set_bg_gradient_colors(bg1[0], bg1[1], bg1[2], bg2[0], bg2[1], bg2[2]);
      save_occt_view_settings();
    }
    const char* gradient_items[] = {"Horizontal", "Vertical", "Diagonal 1", "Diagonal 2",
                                    "Corner 1", "Corner 2", "Corner 3", "Corner 4"};
    int         grad             = m_view->get_bg_gradient_method();
    if (ImGui::Combo("Gradient blend", &grad, gradient_items, 8))
    {
      m_view->set_bg_gradient_method(grad);
      save_occt_view_settings();
    }
  }

  if (ImGui::CollapsingHeader("3D view grid"))
  {
    float g1[3], g2[3];
    m_view->get_grid_colors(g1, g2);
    bool grid_changed = false;
    if (ImGui::ColorEdit3("Grid color 1", g1, ImGuiColorEditFlags_Float))
      grid_changed = true;
    if (ImGui::ColorEdit3("Grid color 2", g2, ImGuiColorEditFlags_Float))
      grid_changed = true;
    if (grid_changed)
    {
      m_view->set_grid_colors(g1[0], g1[1], g1[2], g2[0], g2[1], g2[2]);
      save_occt_view_settings();
    }
  }

  ImGui::Separator();
  if (ImGui::Button("Defaults"))
  {
    std::string content = settings::load_defaults();
    if (content.empty())
      show_message("Failed to load default settings.");
    else
    {
      try
      {
        using namespace nlohmann;
        json j       = json::parse(content);
        j["version"] = k_settings_version;
        content      = j.dump(2);
        settings::save(content);
        parse_occt_view_settings_(content);
        parse_gui_panes_settings_(content);
        if (j.contains("imgui_ini") && j["imgui_ini"].is_string())
        {
          const std::string& ini = j["imgui_ini"].get<std::string>();
          if (!ini.empty())
            ImGui::LoadIniSettingsFromMemory(ini.c_str(), ini.size());
        }
        show_message("Default settings applied.");
      }
      catch (...)
      {
        show_message("Failed to apply default settings.");
      }
    }
  }

  ImGui::End();
}

void GUI::options_normal_mode_()
{
  constexpr std::array<std::string_view, 9> c_names_TopAbs_ShapeEnum =
      {
          "COMPOUND",
          "COMPSOLID",
          "SOLID",
          "SHELL",
          "FACE",
          "WIRE",
          "EDGE",
          "VERTEX",
          "SHAPE"};

  // Shape type filter combo
  int current_item = static_cast<int>(m_view->get_shp_selection_mode());
  if (ImGui::BeginCombo("Selection Mode##filter", c_names_TopAbs_ShapeEnum[current_item].data(),
                        ImGuiComboFlags_WidthFitPreview | ImGuiComboFlags_HeightSmall))
  {
    for (int i = 0; i < static_cast<int>(c_names_TopAbs_ShapeEnum.size()); i++)
      if (ImGui::Selectable(c_names_TopAbs_ShapeEnum[i].data(), current_item == i))
        m_view->set_shp_selection_mode(static_cast<TopAbs_ShapeEnum>(i));

    ImGui::EndCombo();
  }
}

void GUI::options_move_mode_()
{
  ImGui::TextUnformatted("Move constrain axis:");

  Move_options& opts = m_view->shp_move().get_opts();

  ImGui::Checkbox("X", &opts.constr_axis_x);
  ImGui::SameLine();
  ImGui::Checkbox("Y", &opts.constr_axis_y);
  ImGui::SameLine();
  ImGui::Checkbox("Z", &opts.constr_axis_z);
}

void GUI::options_scale_mode_()
{
}

void GUI::options_rotate_mode_()
{
  ImGui::Text("Rotation Options");
  ImGui::Separator();

  // Use radio buttons for axis selection
  int selected_axis = static_cast<int>(m_view->shp_rotate().get_rotation_axis());
  if (ImGui::RadioButton("View to object axis", &selected_axis, static_cast<int>(Rotation_axis::View_to_object)))
    m_view->shp_rotate().set_rotation_axis(Rotation_axis::View_to_object);

  if (ImGui::RadioButton("Around X axis", &selected_axis, static_cast<int>(Rotation_axis::X_axis)))
    m_view->shp_rotate().set_rotation_axis(Rotation_axis::X_axis);

  if (ImGui::RadioButton("Around Y axis", &selected_axis, static_cast<int>(Rotation_axis::Y_axis)))
    m_view->shp_rotate().set_rotation_axis(Rotation_axis::Y_axis);

  if (ImGui::RadioButton("Around Z axis", &selected_axis, static_cast<int>(Rotation_axis::Z_axis)))
    m_view->shp_rotate().set_rotation_axis(Rotation_axis::Z_axis);
}

void GUI::options_sketch_operation_axis_mode_()
{
  if (m_view->curr_sketch().has_operation_axis())
  {
    if (ImGui::Button("Mirror"))
      m_view->curr_sketch().mirror_selected_edges();

    static float revolve_angle = 360.0;

    if (ImGui::Button("Revolve"))
      m_view->revolve_selected(to_radians(revolve_angle));

    ImGui::SetNextItemWidth(80.0f);
    ImGui::SameLine();
    ImGui::InputFloat("##float_value", &revolve_angle, 0.0f, 0.0f, "%.2f");

    if (ImGui::Button("Clear axis"))
      m_view->curr_sketch().clear_operation_axis();
  }
}

void GUI::options_shape_chamfer_mode_()
{
  // Dropdown for Chamfer_mode
  int current_mode = static_cast<int>(m_chamfer_mode);
  if (ImGui::Combo("Chamfer Mode", &current_mode, c_chamfer_mode_strs.data(), (int) c_chamfer_mode_strs.size()))
  {
    m_chamfer_mode = static_cast<Chamfer_mode>(current_mode);
    m_view->on_chamfer_mode();
  }

  // Convert from geometry units to display units for GUI
  float chamfer_dist = float(m_view->shp_chamfer().get_chamfer_dist() / m_view->get_dimension_scale());
  if (ImGui::InputFloat("Chamfer dist##float_value", &chamfer_dist, 0.0f, 0.0f, "%.2f"))
  {
    // Convert from display units to geometry units
    m_view->shp_chamfer().set_chamfer_dist(chamfer_dist * m_view->get_dimension_scale());
  }
}

void GUI::options_shape_fillet_mode_()
{
  // Dropdown for Fillet_mode
  int current_mode = static_cast<int>(m_fillet_mode);
  if (ImGui::Combo("Fillet Mode", &current_mode, c_fillet_mode_strs.data(), (int) c_fillet_mode_strs.size()))
  {
    m_fillet_mode = static_cast<Fillet_mode>(current_mode);
    m_view->on_fillet_mode();
  }

  // Convert from geometry units to display units for GUI
  float fillet_radius = float(m_view->shp_fillet().get_fillet_radius() / m_view->get_dimension_scale());
  if (ImGui::InputFloat("Fillet radius##float_value", &fillet_radius, 0.0f, 0.0f, "%.2f"))
  {
    // Convert from display units to geometry units
    m_view->shp_fillet().set_fillet_radius(fillet_radius * m_view->get_dimension_scale());
  }
}

void GUI::options_shape_polar_duplicate_mode_()
{
  auto& polar_dup    = m_view->shp_polar_dup();
  float polar_angle  = float(polar_dup.get_polar_angle());
  int   num_elms     = int(polar_dup.get_num_elms());
  bool  rotate_dups  = polar_dup.get_rotate_dups();
  bool  combine_dups = polar_dup.get_combine_dups();

  if (ImGui::InputFloat("Polar angle##float_value", &polar_angle, 0.0f, 0.0f, "%.2f"))
    polar_dup.set_polar_angle(polar_angle);

  if (ImGui::InputInt("Num Elms##int_value", &num_elms))
    polar_dup.set_num_elms(num_elms);

  if (ImGui::Checkbox("Rotate dups", &rotate_dups))
    polar_dup.set_rotate_dups(rotate_dups);

  if (ImGui::Checkbox("Combine dups", &combine_dups))
    polar_dup.set_combine_dups(combine_dups);

  if (ImGui::Button("Dup"))
    if (Status s = polar_dup.dup(); !s.is_ok())
      show_message(s.message());
}

void GUI::on_key_rotate_mode_(int key)
{
  const ScreenCoords screen_coords(glm::dvec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y));

  switch (key)
  {
    case GLFW_KEY_ESCAPE:
      m_view->shp_rotate().cancel();
      break;

    case GLFW_KEY_ENTER:
    case GLFW_KEY_KP_ENTER:
      m_view->shp_rotate().finalize();
      break;

    case GLFW_KEY_TAB:
      // Show angle input dialog
      if (Status s = m_view->shp_rotate().show_angle_edit(screen_coords); !s.is_ok())
        show_message(s.message());
      break;

    case GLFW_KEY_X:
      // Toggle between X axis and View_to_object
      if (m_view->shp_rotate().get_rotation_axis() == Rotation_axis::X_axis)
        m_view->shp_rotate().set_rotation_axis(Rotation_axis::View_to_object);
      else
        m_view->shp_rotate().set_rotation_axis(Rotation_axis::X_axis);
      break;

    case GLFW_KEY_Y:
      // Toggle between Y axis and View_to_object
      if (m_view->shp_rotate().get_rotation_axis() == Rotation_axis::Y_axis)
        m_view->shp_rotate().set_rotation_axis(Rotation_axis::View_to_object);
      else
        m_view->shp_rotate().set_rotation_axis(Rotation_axis::Y_axis);
      break;

    case GLFW_KEY_Z:
      // Toggle between Z axis and View_to_object
      if (m_view->shp_rotate().get_rotation_axis() == Rotation_axis::Z_axis)
        m_view->shp_rotate().set_rotation_axis(Rotation_axis::View_to_object);
      else
        m_view->shp_rotate().set_rotation_axis(Rotation_axis::Z_axis);
      break;
  }
}

void GUI::on_key_move_mode_(int key)
{
  Move_options&      opts = m_view->shp_move().get_opts();
  const ScreenCoords screen_coords(glm::dvec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y));

  switch (key)
  {
    case GLFW_KEY_X:
      opts.constr_axis_x ^= 1;
      break;
    case GLFW_KEY_Y:
      opts.constr_axis_y ^= 1;
      break;
    case GLFW_KEY_Z:
      opts.constr_axis_z ^= 1;
      break;
    case GLFW_KEY_TAB:
      m_view->shp_move().show_dist_edit(screen_coords);
      break;
    default:
      break;
  }
}

#ifndef NDEBUG
void GUI::dbg_()
{
  if (!m_show_dbg)
    return;

  if (!ImGui::Begin("dbg", &m_show_dbg))
  {
    ImGui::End();
    return;
  }
  // Get the available content region width
  float available_width = ImGui::GetContentRegionAvail().x;

  // Set the text wrapping position to match the available width
  ImGui::PushTextWrapPos(available_width);
  static std::string dbg_str;
  dbg_str.clear();
  m_view->curr_sketch().dbg_append_str(dbg_str);
  // Display the multiline text with wrapping
  ImGui::TextWrapped("%s", dbg_str.c_str());

  // Reset the wrapping position
  ImGui::PopTextWrapPos();
  ImGui::End();
}
#else
void GUI::dbg_() {}
#endif

void GUI::show_message(const std::string& message)
{
  m_message            = message;
  m_message_visible    = true;
  m_message_start_time = std::chrono::steady_clock::now();
}

void GUI::message_status_window_()
{
  if (!m_message_visible || m_message.empty())
    return;

  // Check if 3 seconds have passed
  auto now     = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_message_start_time).count();
  if (elapsed > 3000)  // 3 seconds
  {
    m_message_visible = false;
    m_message.clear();
    return;
  }

  // Calculate fade effect (optional, for visual polish)
  float alpha = 1.0f;
  if (elapsed > 2500)                             // Start fading after 2.5 seconds
    alpha = 1.0f - (elapsed - 2500.0f) / 500.0f;  // Fade over last 0.5 seconds

  // Set window position (bottom-right corner)
  ImGuiIO& io = ImGui::GetIO();
  ImVec2   window_pos(io.DisplaySize.x - 250.0f, io.DisplaySize.y - 50.0f);  // Adjust as needed
  ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(240.0f, 40.0f), ImGuiCond_Once);

  // Begin window with minimal flags
  ImGui::Begin("MessageStatus##unique_id", nullptr,
               ImGuiWindowFlags_NoTitleBar |
                   ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoFocusOnAppearing |
                   ImGuiWindowFlags_NoNav);

  // Set text color with alpha for fade effect
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, alpha));
  ImGui::TextWrapped("%s", m_message.c_str());
  ImGui::PopStyleColor();

  ImGui::End();
}

void GUI::add_box_dialog_()
{
  if (m_open_add_box_popup)
  {
    ImGui::OpenPopup("Add box");
    m_open_add_box_popup = false;
  }
  if (!ImGui::BeginPopupModal("Add box", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    return;

  ImGui::TextUnformatted("Values in display units.");
  ImGui::Spacing();

  if (ImGui::BeginTable("Add box##table", 2, ImGuiTableFlags_SizingStretchProp))
  {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin X");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##box_origin_x", &m_add_box_origin_x, 0.0, 0.0, "%.3f");

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Y");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##box_origin_y", &m_add_box_origin_y, 0.0, 0.0, "%.3f");

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Z");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##box_origin_z", &m_add_box_origin_z, 0.0, 0.0, "%.3f");

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Width (X)");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##box_width", &m_add_box_width, 0.0, 0.0, "%.3f");

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Length (Y)");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##box_length", &m_add_box_length, 0.0, 0.0, "%.3f");

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Height (Z)");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##box_height", &m_add_box_height, 0.0, 0.0, "%.3f");

    ImGui::EndTable();
  }
  ImGui::Spacing();

  if (ImGui::Button("Add"))
  {
    if (m_add_box_width > 0 && m_add_box_length > 0 && m_add_box_height > 0)
    {
      const double scale = m_view->get_dimension_scale();
      m_view->add_box(
          m_add_box_origin_x * scale, m_add_box_origin_y * scale, m_add_box_origin_z * scale,
          m_add_box_width * scale, m_add_box_length * scale, m_add_box_height * scale);
      ImGui::CloseCurrentPopup();
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel"))
    ImGui::CloseCurrentPopup();

  ImGui::EndPopup();
}

void GUI::add_pyramid_dialog_()
{
  if (m_open_add_pyramid_popup)
  {
    ImGui::OpenPopup("Add pyramid");
    m_open_add_pyramid_popup = false;
  }
  if (!ImGui::BeginPopupModal("Add pyramid", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    return;
  ImGui::TextUnformatted("Values in display units.");
  ImGui::Spacing();
  if (ImGui::BeginTable("Add pyramid##table", 2, ImGuiTableFlags_SizingStretchProp))
  {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin X");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##pyramid_origin_x", &m_add_pyramid_origin_x, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Y");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##pyramid_origin_y", &m_add_pyramid_origin_y, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Z");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##pyramid_origin_z", &m_add_pyramid_origin_z, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Side (base & height)");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##pyramid_side", &m_add_pyramid_side, 0.0, 0.0, "%.3f");
    ImGui::EndTable();
  }
  ImGui::Spacing();
  if (ImGui::Button("Add") && m_add_pyramid_side > 0)
  {
    const double scale = m_view->get_dimension_scale();
    m_view->add_pyramid(m_add_pyramid_origin_x * scale, m_add_pyramid_origin_y * scale, m_add_pyramid_origin_z * scale, m_add_pyramid_side * scale);
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel"))
    ImGui::CloseCurrentPopup();
  ImGui::EndPopup();
}

void GUI::add_sphere_dialog_()
{
  if (m_open_add_sphere_popup)
  {
    ImGui::OpenPopup("Add sphere");
    m_open_add_sphere_popup = false;
  }
  if (!ImGui::BeginPopupModal("Add sphere", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    return;
  ImGui::TextUnformatted("Values in display units.");
  ImGui::Spacing();
  if (ImGui::BeginTable("Add sphere##table", 2, ImGuiTableFlags_SizingStretchProp))
  {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin X");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##sphere_origin_x", &m_add_sphere_origin_x, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Y");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##sphere_origin_y", &m_add_sphere_origin_y, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Z");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##sphere_origin_z", &m_add_sphere_origin_z, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Radius");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##sphere_radius", &m_add_sphere_radius, 0.0, 0.0, "%.3f");
    ImGui::EndTable();
  }
  ImGui::Spacing();
  if (ImGui::Button("Add") && m_add_sphere_radius > 0)
  {
    const double scale = m_view->get_dimension_scale();
    m_view->add_sphere(m_add_sphere_origin_x * scale, m_add_sphere_origin_y * scale, m_add_sphere_origin_z * scale, m_add_sphere_radius * scale);
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel"))
    ImGui::CloseCurrentPopup();
  ImGui::EndPopup();
}

void GUI::add_cylinder_dialog_()
{
  if (m_open_add_cylinder_popup)
  {
    ImGui::OpenPopup("Add cylinder");
    m_open_add_cylinder_popup = false;
  }
  if (!ImGui::BeginPopupModal("Add cylinder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    return;
  ImGui::TextUnformatted("Values in display units.");
  ImGui::Spacing();
  if (ImGui::BeginTable("Add cylinder##table", 2, ImGuiTableFlags_SizingStretchProp))
  {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin X");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##cyl_origin_x", &m_add_cylinder_origin_x, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Y");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##cyl_origin_y", &m_add_cylinder_origin_y, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Z");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##cyl_origin_z", &m_add_cylinder_origin_z, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Radius");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##cyl_radius", &m_add_cylinder_radius, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Height");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##cyl_height", &m_add_cylinder_height, 0.0, 0.0, "%.3f");
    ImGui::EndTable();
  }
  ImGui::Spacing();
  if (ImGui::Button("Add") && m_add_cylinder_radius > 0 && m_add_cylinder_height > 0)
  {
    const double scale = m_view->get_dimension_scale();
    m_view->add_cylinder(m_add_cylinder_origin_x * scale, m_add_cylinder_origin_y * scale, m_add_cylinder_origin_z * scale, m_add_cylinder_radius * scale, m_add_cylinder_height * scale);
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel"))
    ImGui::CloseCurrentPopup();
  ImGui::EndPopup();
}

void GUI::add_cone_dialog_()
{
  if (m_open_add_cone_popup)
  {
    ImGui::OpenPopup("Add cone");
    m_open_add_cone_popup = false;
  }
  if (!ImGui::BeginPopupModal("Add cone", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    return;
  ImGui::TextUnformatted("Values in display units.");
  ImGui::Spacing();
  if (ImGui::BeginTable("Add cone##table", 2, ImGuiTableFlags_SizingStretchProp))
  {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin X");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##cone_origin_x", &m_add_cone_origin_x, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Y");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##cone_origin_y", &m_add_cone_origin_y, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Z");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##cone_origin_z", &m_add_cone_origin_z, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Base radius (R1)");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##cone_R1", &m_add_cone_R1, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Top radius (R2)");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##cone_R2", &m_add_cone_R2, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Height");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##cone_height", &m_add_cone_height, 0.0, 0.0, "%.3f");
    ImGui::EndTable();
  }
  ImGui::Spacing();
  if (ImGui::Button("Add") && m_add_cone_R1 >= 0 && m_add_cone_R2 >= 0 && m_add_cone_height > 0)
  {
    const double scale = m_view->get_dimension_scale();
    m_view->add_cone(m_add_cone_origin_x * scale, m_add_cone_origin_y * scale, m_add_cone_origin_z * scale, m_add_cone_R1 * scale, m_add_cone_R2 * scale, m_add_cone_height * scale);
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel"))
    ImGui::CloseCurrentPopup();
  ImGui::EndPopup();
}

void GUI::add_torus_dialog_()
{
  if (m_open_add_torus_popup)
  {
    ImGui::OpenPopup("Add torus");
    m_open_add_torus_popup = false;
  }
  if (!ImGui::BeginPopupModal("Add torus", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    return;
  ImGui::TextUnformatted("Values in display units.");
  ImGui::Spacing();
  if (ImGui::BeginTable("Add torus##table", 2, ImGuiTableFlags_SizingStretchProp))
  {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin X");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##torus_origin_x", &m_add_torus_origin_x, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Y");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##torus_origin_y", &m_add_torus_origin_y, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Z");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##torus_origin_z", &m_add_torus_origin_z, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Major radius (R1)");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##torus_R1", &m_add_torus_R1, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Minor radius (R2)");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##torus_R2", &m_add_torus_R2, 0.0, 0.0, "%.3f");
    ImGui::EndTable();
  }
  ImGui::Spacing();
  if (ImGui::Button("Add") && m_add_torus_R1 > 0 && m_add_torus_R2 > 0)
  {
    const double scale = m_view->get_dimension_scale();
    m_view->add_torus(m_add_torus_origin_x * scale, m_add_torus_origin_y * scale, m_add_torus_origin_z * scale, m_add_torus_R1 * scale, m_add_torus_R2 * scale);
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel"))
    ImGui::CloseCurrentPopup();
  ImGui::EndPopup();
}

// Log window implementation
void GUI::log_message(const std::string& message)
{
  m_log_messages.push_back(message);
  m_log_scroll_to_bottom = true;  // Auto-scroll to new message
}

void GUI::log_window_()
{
  if (!m_log_window_visible)
    return;

  if (!ImGui::Begin("Log", &m_log_window_visible))
  {
    ImGui::End();
    return;
  }

  // Scrollable log area
  ImGui::BeginChild("LogContent", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
  for (const auto& message : m_log_messages)
    ImGui::TextWrapped("%s", message.c_str());

  // Auto-scroll to bottom if new messages are added
  if (m_log_scroll_to_bottom && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
  {
    ImGui::SetScrollHereY(1.0f);
    m_log_scroll_to_bottom = false;
  }

  ImGui::EndChild();
  ImGui::End();
}

void GUI::lua_console_()
{
  if (!m_show_lua_console)
    return;
  if (!m_lua_console)
    m_lua_console = std::make_unique<Lua_console>(this);
  m_lua_console->render(&m_show_lua_console);
}

void GUI::init(GLFWwindow* window)
{
  initialize_toolbar_();
  settings::set_log_callback([this](const std::string& m)
                             { log_message(m); });
  setup_log_redirection_();  // Set up stream redirection
  m_view->init_window(window);
  m_view->init_viewer();
  m_view->init_default();

  load_occt_view_settings_();
  load_examples_list_();
}

void GUI::on_mouse_pos(const ScreenCoords& screen_coords)
{
  m_view->on_mouse_move(screen_coords);

  switch (get_mode())
  {
    case Mode::Move:
      if (Status s = m_view->shp_move().move_selected(screen_coords); !s.is_ok())
        show_message(s.message());

      break;

    case Mode::Rotate:
      if (Status s = m_view->shp_rotate().rotate_selected(screen_coords); !s.is_ok())
        show_message(s.message());

      break;

    case Mode::Scale:
      if (Status s = m_view->shp_scale().scale_selected(screen_coords); !s.is_ok())
        show_message(s.message());

      break;

    case Mode::Shape_polar_duplicate:
      if (Status s = m_view->shp_polar_dup().move_point(screen_coords); !s.is_ok())
        show_message(s.message());

      break;

    case Mode::Sketch_add_edge:
    case Mode::Sketch_add_multi_edges:
    case Mode::Sketch_operation_axis:
    case Mode::Sketch_add_square:
    case Mode::Sketch_add_rectangle:
    case Mode::Sketch_add_rectangle_center_pt:
    case Mode::Sketch_add_circle:
    case Mode::Sketch_add_slot:
    case Mode::Sketch_add_seg_circle_arc:
      m_view->curr_sketch().sketch_pt_move(screen_coords);
      break;
    case Mode::Sketch_face_extrude:
      m_view->sketch_face_extrude(screen_coords, true);
      break;
    default:
      break;
  }
}

void GUI::on_mouse_button(int button, int action, int mods)
{
  m_view->on_mouse_button(button, action, mods);

  const ScreenCoords screen_coords(glm::dvec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y));

  m_view->on_mouse_move(screen_coords);
  m_view->ctx().UpdateCurrentViewer();

  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && mods == 0)
    switch (m_mode)
    {
      case Mode::Move:
        m_view->shp_move().finalize();
        break;

      case Mode::Rotate:
        m_view->shp_rotate().finalize();
        break;

      case Mode::Scale:
        m_view->shp_scale().finalize();
        break;

      case Mode::Sketch_add_edge:
      case Mode::Sketch_add_multi_edges:
      case Mode::Sketch_add_seg_circle_arc:
      case Mode::Sketch_add_square:
      case Mode::Sketch_add_rectangle:
      case Mode::Sketch_operation_axis:
      case Mode::Sketch_add_circle:
      case Mode::Sketch_add_slot:
        hide_dist_edit();
        m_view->curr_sketch().add_sketch_pt(screen_coords);
        break;

      case Mode::Sketch_face_extrude:
        m_view->sketch_face_extrude(screen_coords, false);
        break;

      case Mode::Shape_chamfer:
        if (Status s = m_view->shp_chamfer().add_chamfer(screen_coords, m_chamfer_mode); !s.is_ok())
          show_message(s.message());

        break;

      case Mode::Shape_fillet:
        if (Status s = m_view->shp_fillet().add_fillet(screen_coords, m_fillet_mode); !s.is_ok())
          show_message(s.message());

        break;

      case Mode::Shape_polar_duplicate:
        if (Status s = m_view->shp_polar_dup().add_point(screen_coords); !s.is_ok())
          show_message(s.message());

        break;

      default:
        break;
    }
  else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && mods == 0)
    switch (m_mode)
    {
      case Mode::Sketch_add_edge:
      case Mode::Sketch_add_multi_edges:
        m_view->curr_sketch().finalize_elm();
        break;

      default:
        break;
    }

  // m_view->on_mouse_button(button, action, mods);
}

void GUI::on_mouse_scroll(double xoffset, double yoffset)
{
  m_view->on_mouse_scroll(xoffset, yoffset);
}

void GUI::on_resize(int width, int height)
{
  m_view->on_resize(width, height);
}

void GUI::setup_log_redirection_()
{
  // Store original stream buffers
  m_original_cout_buf = std::cout.rdbuf();
  m_original_cerr_buf = std::cerr.rdbuf();

  // Create custom stream buffers
  m_cout_log_buf = new Log_strm(*this, m_original_cout_buf);
  m_cerr_log_buf = new Log_strm(*this, m_original_cerr_buf);

  // Redirect stdout and stderr
  std::cout.rdbuf(m_cout_log_buf);
  std::cerr.rdbuf(m_cerr_log_buf);
}

void GUI::cleanup_log_redirection_()
{
  // Restore original stream buffers
  if (m_original_cout_buf)
    std::cout.rdbuf(m_original_cout_buf);
  if (m_original_cerr_buf)
    std::cerr.rdbuf(m_original_cerr_buf);

  // Delete custom stream buffers
  delete m_cout_log_buf;
  delete m_cerr_log_buf;

  m_cout_log_buf      = nullptr;
  m_cerr_log_buf      = nullptr;
  m_original_cout_buf = nullptr;
  m_original_cerr_buf = nullptr;
}

void GUI::import_file_dialog_()
{
#ifndef __EMSCRIPTEN__
  // Native: Use tinyfiledialogs
  char const* filter_patterns[1] = {"*.step"};  // Restrict to .ezy files
  char const* selected           = tinyfd_openFileDialog(
      "Import Step file",  // Dialog title
      "",                  // Default path (empty for OS default)
      1,                   // Number of filter patterns
      filter_patterns,     // Filter patterns (*.step)
      "Step Files",        // Filter description
      0                    // Single file selection
  );
  if (selected)
  {
    std::ifstream     file(selected);
    const std::string step_str {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    if (file.good() && step_str != "")
      on_import_file(selected, step_str);

    else
      show_message("Error opening: " + std::filesystem::path(selected).filename().string());
  }
#else
  // Emscripten: Call async version (synchronous fallback for simplicity)
  open_file_dialog_async("Open EzyCad project");
#endif
}

void GUI::open_file_dialog_()
{
#ifndef __EMSCRIPTEN__
  // Native: Use tinyfiledialogs
  char const* filter_patterns[1] = {"*.ezy"};  // Restrict to .ezy files
  char const* selected           = tinyfd_openFileDialog(
      "Open EzyCad project",  // Dialog title
      "",                     // Default path (empty for OS default)
      1,                      // Number of filter patterns
      filter_patterns,        // Filter patterns (*.ezy)
      "EzyCad Files",         // Filter description
      0                       // Single file selection
  );
  if (selected)
  {
    std::ifstream     file(selected);
    const std::string json_str {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    if (file.good() && json_str != "")
      on_file(selected, json_str);

    else
      show_message("Error opening: " + std::filesystem::path(selected).filename().string());
  }
#else
  // Emscripten: Call async version (synchronous fallback for simplicity)
  open_file_dialog_async("Open EzyCad project");
#endif
}

void GUI::save_file_dialog_()
{
  using namespace nlohmann;
  std::string project_json = m_view->to_json();
  json j = json::parse(project_json);
  j["mode"] = static_cast<int>(get_mode());
  const std::string json_str = j.dump(2);

#ifndef __EMSCRIPTEN__
  std::string file;
  if (!m_last_saved_path.empty())
    file = m_last_saved_path;  // Reuse last saved path
  else
  {
    char const* filter_patterns[1] = {"*.ezy"};
    char const* selected           = tinyfd_saveFileDialog(
        "Save EzyCad Project", "project.ezy", 1, filter_patterns, "EzyCad Files");
    if (selected)
    {
      file              = selected;
      m_last_saved_path = file;  // Update last saved path
    }
  }
  if (!file.empty())
  {
    std::ofstream out(file, std::ios::binary);
    if (out.is_open())
    {
      out.write(json_str.data(), json_str.size());
      out.close();
      show_message("Saved: " + std::filesystem::path(file).filename().string());
    }
    else
      show_message("Failed to save: " + std::filesystem::path(file).filename().string());
  }
  else
    show_message("Save canceled");
#else
  std::string default_file = m_last_saved_path.empty() ? "project.ezy" : std::filesystem::path(m_last_saved_path).filename().string();
  save_file_dialog_async("Save EzyCad project", default_file, json_str);
#endif
}

void GUI::on_file(const std::string& file_path, const std::string& json_str)
{
  using namespace nlohmann;
  const json j = json::parse(json_str);
  if (j.contains("mode") && j["mode"].is_number_integer())
  {
    const int idx = j["mode"].get<int>();
    if (idx >= 0 && idx < static_cast<int>(Mode::_count))
      set_mode(static_cast<Mode>(idx));
  }
  m_view->load(json_str);
  m_last_saved_path = file_path;
  show_message("Opened: " + std::filesystem::path(file_path).filename().string());
}

void GUI::on_import_file(const std::string& file_path, const std::string& file_data)
{
  m_view->import_step(file_data);
  show_message("Imported: " + std::filesystem::path(file_path).filename().string());
}

#ifdef __EMSCRIPTEN__
void GUI::open_file_dialog_async(const char* title)
{
  EM_ASM_ARGS({
    var input           = document.createElement('input');
    input.type          = 'file';
    input.accept        = '.ezy';
    input.style.display = 'none';
    document.body.appendChild(input);
    input.onchange = function(e)
    {
      var file = e.target.files[0];
      if (file)
      {
        var reader    = new FileReader();
        reader.onload = function(e)
        {
          var contents    = new Uint8Array(e.target.result);
          var fileName    = file.name;
          // Allocate heap memory for contents
          var length      = contents.length;
          var contentsPtr = _malloc(length);
          HEAPU8.set(contents, contentsPtr);
          // Call C++ callback with path and contents
          Module.ccall('on_file_selected', null, ['string', 'number', 'number'], [fileName, contentsPtr, length]);
          _free(contentsPtr);
        };
        reader.readAsArrayBuffer(file);
      }
      document.body.removeChild(input);
    };
    input.click();
  });
}

void GUI::save_file_dialog_async(const char* title, const std::string& default_file, const std::string& json_str)
{
  EM_ASM_ARGS({
      var data = HEAPU8.subarray($0, $0 + $1);
      var blob = new Blob([data], { type: 'application/octet-stream' });
      var url = URL.createObjectURL(blob);
      var a = document.createElement('a');
      a.href = url;
      a.download = UTF8ToString($2); // Default file name
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      URL.revokeObjectURL(url);
      Module.ccall('on_save_file_selected', null, ['string'], [UTF8ToString($2)]); }, json_str.data(), json_str.size(), default_file.c_str());
}

// C-style callback for Emscripten
extern "C" void on_file_selected(const char* file_path, char* contents, int length)
{
  const std::string json_str(contents, length);
  GUI::instance().on_file(file_path, json_str);
}

extern "C" void on_save_file_selected(const char* file_name)
{
  GUI::instance().show_message(std::string("Saved: ") + file_name);
}

#endif
