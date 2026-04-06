#include "gui.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_set>

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
#include "python_console.h"
#include "sketch.h"

// Must be here to prevent compiler warning
#include <GLFW/glfw3.h>

#ifndef __EMSCRIPTEN__
#include <cstdlib>
#endif

static bool is_valid_project_json(const std::string& s)
{
  try
  {
    const nlohmann::json j = nlohmann::json::parse(s);
    return j.contains("sketches") && j["sketches"].is_array();
  }
  catch (...)
  {
    return false;
  }
}

namespace
{

// Shape List: when a row matches OCCT selection, ImGuiCol_Text is nudged brighter (RGB only).
constexpr float k_shape_list_selected_text_rgb_scale =
    1.08f;  // per-channel multiplier for a modest relative lift
constexpr float k_shape_list_selected_text_rgb_bias =
    0.04f;  // added after scaling so very dark text still reads a bit lighter

}  // namespace

const std::vector<std::string>& occt_material_combo_labels()
{
  static std::vector<std::string> names;
  if (names.empty())
    for (int i = 0; i < Graphic3d_MaterialAspect::NumberOfMaterials(); ++i)
    {
      Graphic3d_MaterialAspect mat(static_cast<Graphic3d_NameOfMaterial>(i));
      names.emplace_back(mat.MaterialName());
    }
  return names;
}

GUI* gui_instance = nullptr;

GUI::GUI()
{
  m_view = std::make_unique<Occt_view>(*this);
  EZY_ASSERT(!gui_instance);
  gui_instance = this;
}

ImFont* GUI::console_font() const
{
  ImFont* f = m_console_font;
  if (!f || !ImGui::GetCurrentContext())
    return nullptr;
  ImFontAtlas* atlas = ImGui::GetIO().Fonts;
  if (!atlas)
    return nullptr;
  for (int i = 0; i < atlas->Fonts.Size; ++i)
  {
    if (atlas->Fonts[i] == f)
      return f;
  }
  return nullptr;
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
  // Underlay transform sliders use sketch_plane_view_aabb_2d → pt_on_plane → view projection.
  // FlushViewEvents must run before ImGui so the camera matches the latest pan/zoom/rotate (do_frame() runs later).
  m_view->flush_view_events();

  if (m_dark_mode)
    ImGui::StyleColorsDark();
  else
    ImGui::StyleColorsLight();
  apply_imgui_rounding_from_members_();

  menu_bar_();
  toolbar_();
  dist_edit_();
  angle_edit_();
  sketch_list_();
  sketch_properties_dialog_();
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
  python_console_();
  settings_();
  dbg_();
}

void GUI::render_occt()
{
  m_view->do_frame();
}

// Initialize toolbar buttons
void GUI::initialize_toolbar_()
{
  m_toolbar_buttons = {
      {                           load_texture("User.png"),  true,"Inspection mode",                         Mode::Normal                                                                  },
      {        load_texture("Workbench_Sketcher_none.png"), false,           "Sketch inspection mode",         Mode::Sketch_inspection_mode},
      {             load_texture("Assembly_AxialMove.png"), false,                   "Shape move (g)",                           Mode::Move},
      {                   load_texture("Draft_Rotate.png"), false,                 "Shape rotate (r)",                         Mode::Rotate},
      {                     load_texture("Part_Scale.png"), false,                  "Shape Scale (s)",                          Mode::Scale},
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
      {       load_texture("TechDraw_LengthDimension.png"), false,
       "Toggle edge dimension annotation (Options: length value placement)",         Mode::Sketch_toggle_edge_dim                          },
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
    m_example_files.push_back(Example_file{std::move(label), std::move(path)});
  }
  std::sort(m_example_files.begin(), m_example_files.end(),
            [](const Example_file& a, const Example_file& b)
            { return a.label < b.label; });
}

void GUI::menu_bar_()
{
  if (!ImGui::BeginMainMenuBar())
    return;

  if (ImGui::BeginMenu("File"))
  {
    // Use separate `if` (not `else if`) for each entry. `BeginMenu` stays true while the submenu
    // is open; chaining with `else if` would skip later items (e.g. Examples hidden while Export open).
    if (ImGui::MenuItem("New", "Ctrl+N"))
      m_view->new_file();

    if (ImGui::MenuItem("Open", "Ctrl+O"))
      open_file_dialog_();

    if (ImGui::MenuItem("Save", "Ctrl+S"))
      save_file_dialog_();

    if (ImGui::MenuItem("Save as"))
    {
      m_last_saved_path.clear();  // Force save as dialog
      save_file_dialog_();
    }

    if (ImGui::MenuItem("Import"))
      import_file_dialog_();

    if (ImGui::MenuItem("Sketch underlay image..."))
    {
      m_underlay_import_sketch_target.reset();
      sketch_underlay_import_dialog_();
    }

    if (ImGui::BeginMenu("Export"))
    {
      if (ImGui::MenuItem("STEP (.step)..."))
        export_file_dialog_(Export_format::Step);

      if (ImGui::MenuItem("IGES (.igs)..."))
        export_file_dialog_(Export_format::Iges);

      if (ImGui::MenuItem("STL (binary)..."))
        export_file_dialog_(Export_format::Stl);

      if (ImGui::MenuItem("PLY (binary)..."))
        export_file_dialog_(Export_format::Ply);

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Examples"))
    {
      for (const Example_file& ex : m_example_files)
        if (ImGui::MenuItem(ex.label.c_str()))
        {
          std::ifstream file(ex.path);
          std::string   json_str {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
          if (file.good() && !json_str.empty())
            on_file(ex.path, json_str);
          else
            show_message("Error opening example: " + ex.label);
        }

      ImGui::EndMenu();
    }
#ifdef __EMSCRIPTEN__
    if (ImGui::MenuItem("Save settings"))
    {
      save_occt_view_settings();
      show_message("Settings saved");
    }
#endif

    if (ImGui::MenuItem("Exit"))
      exit(0);

    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Edit"))
  {
    if (ImGui::MenuItem("Undo", "Ctrl+Z", false, m_view->can_undo()))
      m_view->undo();

    if (ImGui::MenuItem("Redo", "Ctrl+Y", false, m_view->can_redo()))
      m_view->redo();

    ImGui::Separator();
    if (ImGui::MenuItem("Add box"))
    {
      const double scale = m_view->get_dimension_scale();
      m_view->add_box(0, 0, 0, scale, scale, scale);
    }
    if (ImGui::MenuItem("Add box_prms"))
    {
      m_add_box_origin_x   = 0;
      m_add_box_origin_y   = 0;
      m_add_box_origin_z   = 0;
      m_add_box_width      = 1.0;
      m_add_box_length     = 1.0;
      m_add_box_height     = 1.0;
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
      m_add_pyramid_side                                                       = 1.0;
      m_open_add_pyramid_popup                                                 = true;
    }
    if (ImGui::MenuItem("Add sphere"))
    {
      const double scale = m_view->get_dimension_scale();
      m_view->add_sphere(0, 0, 0, scale);
    }
    if (ImGui::MenuItem("Add sphere_prms"))
    {
      m_add_sphere_origin_x = m_add_sphere_origin_y = m_add_sphere_origin_z = 0;
      m_add_sphere_radius                                                   = 1.0;
      m_open_add_sphere_popup                                               = true;
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
      m_open_add_cylinder_popup                     = true;
    }
    if (ImGui::MenuItem("Add cone"))
    {
      const double scale = m_view->get_dimension_scale();
      m_view->add_cone(0, 0, 0, scale, 0.0, scale);
    }
    if (ImGui::MenuItem("Add cone_prms"))
    {
      m_add_cone_origin_x = m_add_cone_origin_y = m_add_cone_origin_z = 0;
      m_add_cone_R1                                                   = 1.0;
      m_add_cone_R2                                                   = 0.0;
      m_add_cone_height                                               = 1.0;
      m_open_add_cone_popup                                           = true;
    }
    if (ImGui::MenuItem("Add torus"))
    {
      const double scale = m_view->get_dimension_scale();
      m_view->add_torus(0, 0, 0, scale, scale / 2.0);
    }
    if (ImGui::MenuItem("Add torus_prms"))
    {
      m_add_torus_origin_x = m_add_torus_origin_y = m_add_torus_origin_z = 0;
      m_add_torus_R1                                                     = 1.0;
      m_add_torus_R2                                                     = 0.5;
      m_open_add_torus_popup                                             = true;
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
    if (ImGui::MenuItem("Lua Console", nullptr, m_show_lua_console))
    {
      m_show_lua_console = !m_show_lua_console;
      save_panes         = true;
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Interactive Lua prompt and res/scripts/lua editors.");
    if (ImGui::MenuItem("Python Console", nullptr, m_show_python_console))
    {
      m_show_python_console = !m_show_python_console;
      save_panes            = true;
    }
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

  m_dist_callback           = std::move(callback);
  m_dist_edit_focus_pending = true;
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

  ImGui::SetNextItemWidth(72.0f);
  // Focusing every frame prevents IsItemDeactivatedAfterEdit (click away / Tab) from ever committing.
  if (m_dist_edit_focus_pending)
  {
    ImGui::SetKeyboardFocusHere(0);
    m_dist_edit_focus_pending = false;
  }

  // Add a small input float widget and check for changes
  if (ImGui::InputFloat("##dist_edit_float_value", &m_dist_val, 0.0f, 0.0f, "%.2f"))
    m_dist_callback(m_dist_val, false);
  else
    m_dist_val = std::round(m_dist_val * 100.0f) / 100.0f;

  if (ImGui::IsItemDeactivatedAfterEdit() && m_dist_callback)
  {
    std::function<void(float, bool)> callback;
    std::swap(callback, m_dist_callback);
    callback(m_dist_val, true);
  }

  ImGui::SameLine();
  if (ImGui::SmallButton("OK") && m_dist_callback)
  {
    std::function<void(float, bool)> callback;
    std::swap(callback, m_dist_callback);
    callback(m_dist_val, true);
  }

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

  m_angle_callback           = std::move(callback);
  m_angle_edit_focus_pending = true;
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

bool GUI::is_dist_or_angle_edit_active() const
{
  return m_dist_callback != nullptr || m_angle_callback != nullptr;
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

  ImGui::SetNextItemWidth(72.0f);
  if (m_angle_edit_focus_pending)
  {
    ImGui::SetKeyboardFocusHere(0);
    m_angle_edit_focus_pending = false;
  }

  // Add a small input float widget and check for changes
  if (ImGui::InputFloat("##angle_edit_float_value", &m_angle_val, 0.0f, 0.0f, "%.2f"))
    m_angle_callback(m_angle_val, false);
  else
    m_angle_val = std::round(m_angle_val * 100.0f) / 100.0f;

  if (ImGui::IsItemDeactivatedAfterEdit() && m_angle_callback)
  {
    std::function<void(float, bool)> callback;
    std::swap(callback, m_angle_callback);
    callback(m_angle_val, true);
  }

  ImGui::SameLine();
  if (ImGui::SmallButton("OK") && m_angle_callback)
  {
    std::function<void(float, bool)> callback;
    std::swap(callback, m_angle_callback);
    callback(m_angle_val, true);
  }

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

    ImGui::SameLine();
    ImGui::PushID(("uldisp" + id_suffix).c_str());
    {
      const bool has_ul = sketch->has_underlay();
      bool       dummy_off {false};
      bool       ul_vis   = has_ul && sketch->underlay_visible();
      if (!has_ul)
        ImGui::BeginDisabled();
      bool* const vptr = has_ul ? &ul_vis : &dummy_off;
      if (ImGui::Checkbox("", vptr) && has_ul)
      {
        sketch->underlay_set_visible(ul_vis);
        if (m_underlay_panel_sketch == sketch.get())
          m_ul_vis = ul_vis;
      }
      if (!has_ul)
        ImGui::EndDisabled();
      if (m_show_tool_tips && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip(
            has_ul ? "Display underlay" : "Import an image in Sketch properties to enable the underlay.");
    }
    ImGui::PopID();

    ImGui::SameLine();
    ImGui::PushID(("props" + id_suffix).c_str());
    if (ImGui::SmallButton("[P]"))
    {
      m_sketch_properties_sketch = sketch;
      m_sketch_properties_open   = true;
    }

    if (m_show_tool_tips && ImGui::IsItemHovered())
      ImGui::SetTooltip("Sketch properties");

    ImGui::PopID();

    ++index;
    ImGui::PopID();
  }

  if (sketch_to_delete)
  {
    if (const auto p = m_sketch_properties_sketch.lock(); p && p == sketch_to_delete)
      m_sketch_properties_open = false;
    m_view->remove_sketch(sketch_to_delete);
  }

  ImGui::End();
}

void GUI::sketch_underlay_import_dialog_()
{
#ifndef __EMSCRIPTEN__
  char const* filter_patterns[4] = {"*.png", "*.jpg", "*.jpeg", "*.bmp"};
  char const* selected           = tinyfd_openFileDialog(
      "Sketch underlay image",
      "",
      4,
      filter_patterns,
      "PNG / JPEG / BMP",
      0);
  if (selected)
  {
    std::ifstream file(selected, std::ios::binary);
    if (!file.is_open())
    {
      show_message("Error opening: " + std::filesystem::path(selected).filename().string());
      return;
    }
    const std::string file_bytes {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    if (!file_bytes.empty())
      on_sketch_underlay_file(selected, file_bytes);
    else
      show_message("Error opening: " + std::filesystem::path(selected).filename().string());
  }
#else
  sketch_underlay_file_dialog_async();
#endif
}

void GUI::on_sketch_underlay_file(const std::string& file_path, const std::string& file_bytes)
{
  std::shared_ptr<Sketch> sk = m_underlay_import_sketch_target.lock();
  m_underlay_import_sketch_target.reset();
  if (!sk)
    sk = m_view->curr_sketch_shared();
  m_view->push_undo_snapshot();
  if (!sk->load_underlay_image(file_bytes))
  {
    show_message("Could not decode image: " + std::filesystem::path(file_path).filename().string());
    return;
  }
  m_underlay_panel_sketch = nullptr;
  show_message("Underlay: " + std::filesystem::path(file_path).filename().string());
}

void GUI::sketch_properties_dialog_()
{
  if (!m_sketch_properties_open)
    return;

  const auto sk = m_sketch_properties_sketch.lock();
  if (!sk)
  {
    m_sketch_properties_open = false;
    return;
  }

  ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_FirstUseEver);
  bool open = m_sketch_properties_open;
  if (!ImGui::Begin("Sketch properties", &open, ImGuiWindowFlags_None))
  {
    m_sketch_properties_open = open;
    ImGui::End();
    return;
  }

  ImGui::TextUnformatted(sk->get_name().c_str());
  ImGui::Separator();
  sketch_underlay_panel_settings_(sk);
  m_sketch_properties_open = open;
  ImGui::End();
}

void GUI::sketch_underlay_panel_settings_(const std::shared_ptr<Sketch>& sk)
{
  EZY_ASSERT(sk);

  ImGui::TextUnformatted("Image underlay");
  ImGui::Separator();

  if (m_underlay_panel_sketch != sk.get())
  {
    m_underlay_panel_sketch = sk.get();
    if (sk->has_underlay())
    {
      sk->underlay_ui_params(m_ul_cx, m_ul_cy, m_ul_hw, m_ul_hh, m_ul_rot);
      m_ul_opacity   = sk->underlay_opacity();
      m_ul_vis       = sk->underlay_visible();
      m_ul_key_white = sk->underlay_key_white_transparent();
      m_ul_line_tint = sk->underlay_line_tint_enabled();
      {
        uint8_t tr, tg, tb;
        sk->underlay_line_tint_rgb(tr, tg, tb);
        m_ul_tint_col[0] = static_cast<float>(tr) / 255.f;
        m_ul_tint_col[1] = static_cast<float>(tg) / 255.f;
        m_ul_tint_col[2] = static_cast<float>(tb) / 255.f;
      }
    }
    else
    {
      m_ul_cx = m_ul_cy = m_ul_hw = m_ul_hh = m_ul_rot = 0.;
      m_ul_opacity                                     = 0.88f;
      m_ul_vis                                         = true;
      m_ul_key_white                                   = true;
      m_ul_line_tint                                   = true;
      m_ul_tint_col[0]                                 = 1.f;
      m_ul_tint_col[1]                                 = 0.863f;
      m_ul_tint_col[2]                                 = 0.f;
    }
  }

#ifndef __EMSCRIPTEN__
  if (ImGui::Button("Import image..."))
  {
    m_underlay_import_sketch_target = sk;
    sketch_underlay_import_dialog_();
  }
#else
  if (ImGui::Button("Import image..."))
  {
    m_underlay_import_sketch_target = sk;
    sketch_underlay_file_dialog_async();
  }
#endif

  ImGui::SameLine();
  if (ImGui::Button("Remove underlay"))
  {
    if (sk->has_underlay())
    {
      m_view->push_undo_snapshot();
      sk->clear_underlay();
      m_underlay_panel_sketch = nullptr;
    }
  }

  ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextDisabled(
        "Import PNG/JPEG/BMP as a sketch underlay. Adjust half-width, half-height, center, and rotation "
        "to match real dimensions; changes apply in real time.");
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }

  if (!sk->has_underlay())
    return;

  if (ImGui::Checkbox("White paper → transparent", &m_ul_key_white))
    sk->underlay_set_key_white_transparent(m_ul_key_white);

  if (m_show_tool_tips && ImGui::IsItemHovered())
    ImGui::SetTooltip(
        "Uses brightness: white background becomes clear; dark lines stay visible. "
        "Turn off for full-color photos. Inverting the image is not needed for typical scans.");

  if (ImGui::Checkbox("Tint visible lines", &m_ul_line_tint))
    sk->underlay_set_line_tint_enabled(m_ul_line_tint);

  if (m_show_tool_tips && ImGui::IsItemHovered())
    ImGui::SetTooltip(
        "Paints non-transparent pixels (after white key) with the line color. "
        "Default yellow reads well on dark backgrounds.");

  if (m_ul_line_tint)
  {
    if (ImGui::ColorEdit3("Line color", m_ul_tint_col))
    {
      const auto to_u8 = [](float c) -> uint8_t
      {
        const float x = std::clamp(c, 0.f, 1.f) * 255.f;
        return static_cast<uint8_t>(x + 0.5f);
      };
      sk->underlay_set_line_tint_rgb(to_u8(m_ul_tint_col[0]), to_u8(m_ul_tint_col[1]), to_u8(m_ul_tint_col[2]));
    }
  }

  if (ImGui::SliderFloat("Opacity", &m_ul_opacity, 0.f, 1.f, "%.2f"))
    sk->underlay_set_opacity(m_ul_opacity);

  ImGui::Separator();
  ImGui::TextUnformatted("Calibrate from sketch edges");
  {
    const bool sk_is_cur = (m_view->curr_sketch_shared() == sk);
    if (!sk_is_cur)
      ImGui::TextDisabled("(Make this sketch current in the sketch list to pick.)");

    const bool pick_x = m_underlay_calib_phase == Underlay_calib_phase::PickX1 ||
                        m_underlay_calib_phase == Underlay_calib_phase::PickX2 ||
                        m_underlay_calib_phase == Underlay_calib_phase::AwaitDistX;
    const bool pick_y = m_underlay_calib_phase == Underlay_calib_phase::PickY1 ||
                        m_underlay_calib_phase == Underlay_calib_phase::PickY2 ||
                        m_underlay_calib_phase == Underlay_calib_phase::AwaitDistY;

    if (pick_x || pick_y)
    {
      const char* hint = "";
      switch (m_underlay_calib_phase)
      {
        case Underlay_calib_phase::PickX1:
          hint = "Click first point: corner at bitmap (0,0).";
          break;
        case Underlay_calib_phase::PickX2:
          hint = "Click second point: end of width (bitmap +U), like a line edge.";
          break;
        case Underlay_calib_phase::AwaitDistX:
          hint = "Enter the drawing distance for X (dimension popup). Y length is set from image aspect (H:W).";
          break;
        case Underlay_calib_phase::PickY1:
          hint = "Click first point along image height (+V).";
          break;
        case Underlay_calib_phase::PickY2:
          hint = "Click second point: end of height (+V).";
          break;
        case Underlay_calib_phase::AwaitDistY:
          hint = "Enter the drawing distance for Y (dimension popup).";
          break;
        default:
          break;
      }
      ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.25f, 1.0f), "%s", hint);
    }

    ImGui::BeginDisabled(!sk_is_cur || pick_y);
    if (ImGui::Button("Set X from edge…"))
      begin_underlay_calib_set_x_(sk);
    ImGui::EndDisabled();
    if (m_show_tool_tips && ImGui::IsItemHovered())
      ImGui::SetTooltip(
          "Two clicks along width (+U), then type the real drawing distance (same units as sketch dimensions). "
          "Y is set from image aspect until you use Set Y.");

    ImGui::SameLine();
    ImGui::BeginDisabled(!sk_is_cur || !m_underlay_calib_have_x || pick_x);
    if (ImGui::Button("Set Y from edge…"))
      begin_underlay_calib_set_y_(sk);
    ImGui::EndDisabled();
    if (m_show_tool_tips && ImGui::IsItemHovered())
      ImGui::SetTooltip("After Set X: two clicks along height (+V), then enter the drawing distance for Y.");

  }

  ImGui::Separator();
  ImGui::TextUnformatted("Transform (sketch plane, vs. current view)");
  {
    const ImGuiIO& io = ImGui::GetIO();
    double           min_u = 0., min_v = 0., max_u = 1., max_v = 1.;
    const bool       have_view =
        m_view->sketch_plane_view_aabb_2d(sk->get_plane(), static_cast<double>(io.DisplaySize.x),
                                          static_cast<double>(io.DisplaySize.y), min_u, min_v, max_u, max_v);
    if (!have_view)
    {
      constexpr double k_fallback = 250.0;
      // m_ul_cx / m_ul_cy stay sketch-plane gp_Pnt2d X / Y (same as underlay_ui_params).
      min_u = m_ul_cx - k_fallback;
      max_u = m_ul_cx + k_fallback;
      min_v = m_ul_cy - k_fallback;
      max_v = m_ul_cy + k_fallback;
    }

    const double span_u = std::max(max_u - min_u, 1e-6);
    const double span_v = std::max(max_v - min_v, 1e-6);
    const double half_span_u = 0.5 * span_u;
    const double half_span_v = 0.5 * span_v;
    // Allow underlay larger than the visible frustum (trace paper / zoomed views).
    const double max_half_w = std::max(std::hypot(half_span_u, half_span_v) * 2.5, 1e-3);
    const double max_half_h = max_half_w;
    constexpr double k_min_half = 1e-6;
    double           rot_min = -180.0;
    double           rot_max = 180.0;

    auto apply_ul_xform = [&]()
    { sk->underlay_set_center_extents_rotation(m_ul_cx, m_ul_cy, m_ul_hw, m_ul_hh, m_ul_rot); };

    auto transform_slider = [&](const char* label, ImGuiDataType type, void* p_data, const void* p_min,
                                const void* p_max, const char* format, ImGuiSliderFlags flags = 0)
    {
      const bool changed = ImGui::SliderScalar(label, type, p_data, p_min, p_max, format, flags);
      if (ImGui::IsItemActivated())
        m_view->push_undo_snapshot();
      if (changed)
        apply_ul_xform();
    };

    // Labels match typical sketch axes on screen: "Center X" drives plane Y (gp_Pnt2d::Y / view v), "Center Y" plane X (u).
    transform_slider("Center X", ImGuiDataType_Double, &m_ul_cy, &min_v, &max_v, "%.4f", ImGuiSliderFlags_ClampOnInput);
    transform_slider("Center Y", ImGuiDataType_Double, &m_ul_cx, &min_u, &max_u, "%.4f", ImGuiSliderFlags_ClampOnInput);
    transform_slider("Half width", ImGuiDataType_Double, &m_ul_hw, &k_min_half, &max_half_w, "%.4f",
                     ImGuiSliderFlags_ClampOnInput | ImGuiSliderFlags_Logarithmic);
    transform_slider("Half height", ImGuiDataType_Double, &m_ul_hh, &k_min_half, &max_half_h, "%.4f",
                     ImGuiSliderFlags_ClampOnInput | ImGuiSliderFlags_Logarithmic);
    transform_slider("Rotation (deg)", ImGuiDataType_Double, &m_ul_rot, &rot_min, &rot_max, "%.1f",
                     ImGuiSliderFlags_ClampOnInput);
  }
}

void GUI::cancel_underlay_calib_()
{
  m_dist_callback = nullptr;
  m_underlay_calib_phase   = Underlay_calib_phase::None;
  m_underlay_calib_sketch_wk.reset();
  m_underlay_calib_have_x  = false;
  m_underlay_calib_axis_u  = gp_Vec2d(0., 0.);
}

void GUI::begin_underlay_calib_set_x_(const std::shared_ptr<Sketch>& sk)
{
  if (!sk || !sk->has_underlay())
    return;
  if (m_view->curr_sketch_shared() != sk)
  {
    show_message("Make this sketch current in the sketch list, then try again.");
    return;
  }
  cancel_underlay_calib_();
  m_underlay_calib_sketch_wk = sk;
  m_underlay_calib_phase     = Underlay_calib_phase::PickX1;
  show_message(
      "Underlay X: click corner (0,0), then point along +U; you will enter the drawing distance for that span.");
}

void GUI::begin_underlay_calib_set_y_(const std::shared_ptr<Sketch>& sk)
{
  if (!sk || !sk->has_underlay())
    return;
  if (!m_underlay_calib_have_x)
  {
    show_message("Use Set X from edge first (two points along image width).");
    return;
  }
  if (m_view->curr_sketch_shared() != sk)
  {
    show_message("Make this sketch current in the sketch list, then try again.");
    return;
  }
  m_underlay_calib_sketch_wk = sk;
  m_underlay_calib_phase     = Underlay_calib_phase::PickY1;
  show_message("Underlay Y: click two points along +V, then enter the drawing distance for that span.");
}

void GUI::underlay_calib_prompt_x_distance_(const std::shared_ptr<Sketch>& sk)
{
  m_underlay_calib_phase = Underlay_calib_phase::AwaitDistX;
  const double L_model   = m_underlay_calib_x0.Distance(m_underlay_calib_x1);
  const float  dist_show = static_cast<float>(L_model / m_view->get_dimension_scale());
  const ScreenCoords spos(glm::dvec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y));

  std::weak_ptr<Sketch> wk = sk;
  auto                  on_dist = [this, wk](float new_dist, bool is_final)
  {
    if (!is_final)
      return;
    const std::shared_ptr<Sketch> s = wk.lock();
    if (!s || !s->has_underlay())
    {
      m_dist_callback = nullptr;
      cancel_underlay_calib_();
      return;
    }
    if (m_underlay_calib_phase != Underlay_calib_phase::AwaitDistX)
      return;

    const double Dx = static_cast<double>(new_dist) * m_view->get_dimension_scale();
    if (Dx <= 1e-12)
    {
      show_message("Distance must be positive.");
      return;
    }

    m_view->push_undo_snapshot();
    if (!s->underlay_rescale_uv_chord_to_length(m_underlay_calib_x0, m_underlay_calib_x1, Dx))
    {
      m_view->pop_undo_snapshot();
      show_message("Could not calibrate X (underlay axes degenerate or segment too short).");
      return;
    }
    m_underlay_calib_axis_u = s->underlay_axis_u_vec();
    s->underlay_ui_params(m_ul_cx, m_ul_cy, m_ul_hw, m_ul_hh, m_ul_rot);
    m_underlay_panel_sketch = nullptr;

    m_dist_callback          = nullptr;
    m_underlay_calib_phase   = Underlay_calib_phase::None;
    m_underlay_calib_have_x  = true;
    show_message("X scale applied to the picked segment. Use Set Y from edge for the vertical span and its distance.");
  };

  set_dist_edit(dist_show, std::move(on_dist), spos);
}

void GUI::underlay_calib_prompt_y_distance_(const std::shared_ptr<Sketch>& sk)
{
  m_underlay_calib_phase = Underlay_calib_phase::AwaitDistY;
  const double L_model   = m_underlay_calib_y0.Distance(m_underlay_calib_y1);
  const float  dist_show = static_cast<float>(L_model / m_view->get_dimension_scale());
  const ScreenCoords spos(glm::dvec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y));

  std::weak_ptr<Sketch> wk = sk;
  auto                  on_dist = [this, wk](float new_dist, bool is_final)
  {
    if (!is_final)
      return;
    const std::shared_ptr<Sketch> s = wk.lock();
    if (!s || !s->has_underlay())
    {
      m_dist_callback = nullptr;
      cancel_underlay_calib_();
      return;
    }
    if (m_underlay_calib_phase != Underlay_calib_phase::AwaitDistY)
      return;

    const double Dy = static_cast<double>(new_dist) * m_view->get_dimension_scale();
    if (Dy <= 1e-12)
    {
      show_message("Distance must be positive.");
      return;
    }

    m_view->push_undo_snapshot();
    if (!s->underlay_rescale_v_chord_to_length(m_underlay_calib_y0, m_underlay_calib_y1, Dy))
    {
      m_view->pop_undo_snapshot();
      show_message(
          "Set Y: picks need a clear span along image height (not along the same edge as X only). Try two points further apart in V.");
      return;
    }
    s->underlay_ui_params(m_ul_cx, m_ul_cy, m_ul_hw, m_ul_hh, m_ul_rot);
    m_underlay_panel_sketch = nullptr;

    m_dist_callback = nullptr;
    cancel_underlay_calib_();
    show_message("Underlay X/Y calibration complete.");
  };

  set_dist_edit(dist_show, std::move(on_dist), spos);
}

bool GUI::try_underlay_calib_click_(const ScreenCoords& screen_coords)
{
  if (m_underlay_calib_phase == Underlay_calib_phase::None)
    return false;

  if (m_underlay_calib_phase == Underlay_calib_phase::AwaitDistX ||
      m_underlay_calib_phase == Underlay_calib_phase::AwaitDistY)
    return true;

  const std::shared_ptr<Sketch> sk     = m_underlay_calib_sketch_wk.lock();
  const std::shared_ptr<Sketch> sk_cur = m_view->curr_sketch_shared();
  if (!sk || !sk_cur || sk != sk_cur)
  {
    cancel_underlay_calib_();
    show_message("Underlay calibration canceled.");
    return true;
  }
  if (!sk->has_underlay())
  {
    cancel_underlay_calib_();
    return true;
  }

  const std::optional<gp_Pnt2d> pt = sk->pick_point_for_underlay_calib(screen_coords);
  if (!pt)
    return true;

  auto too_short = [](const gp_Pnt2d& a, const gp_Pnt2d& b)
  {
    const double dx = b.X() - a.X();
    const double dy = b.Y() - a.Y();
    return dx * dx + dy * dy <= 1e-16;
  };

  switch (m_underlay_calib_phase)
  {
    case Underlay_calib_phase::PickX1:
      m_underlay_calib_x0  = *pt;
      m_underlay_calib_phase = Underlay_calib_phase::PickX2;
      show_message("Underlay X: click second point (end of width / +U).");
      return true;
    case Underlay_calib_phase::PickX2:
      if (too_short(m_underlay_calib_x0, *pt))
      {
        show_message("X segment too short.");
        return true;
      }
      m_underlay_calib_x1 = *pt;
      show_message("Enter drawing distance for X (same unit convention as sketch edge dimensions).");
      underlay_calib_prompt_x_distance_(sk);
      return true;
    case Underlay_calib_phase::PickY1:
      m_underlay_calib_y0  = *pt;
      m_underlay_calib_phase = Underlay_calib_phase::PickY2;
      show_message("Underlay Y: click second point (end of height / +V).");
      return true;
    case Underlay_calib_phase::PickY2:
      if (too_short(m_underlay_calib_y0, *pt))
      {
        show_message("Y segment too short.");
        return true;
      }
      m_underlay_calib_y1 = *pt;
      show_message("Enter drawing distance for Y.");
      underlay_calib_prompt_y_distance_(sk);
      return true;
    default:
      return false;
  }
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
    for (const Shp_ptr& shape : m_view->get_shapes())
    {
      shape->set_visible(!m_hide_all_shapes);
    }
  }

  ImGui::Separator();

  const std::vector<std::string>& mat_names       = occt_material_combo_labels();
  const int                       nmat            = static_cast<int>(mat_names.size());
  float                           mat_label_w_max = 0.0f;
  for (int mi = 0; mi < nmat; ++mi)
    mat_label_w_max =
        std::max(mat_label_w_max, ImGui::CalcTextSize(mat_names[static_cast<size_t>(mi)].c_str()).x);
  const ImGuiStyle& st_mat      = ImGui::GetStyle();
  const float       mat_popup_w = std::min(
      440.0f,
      std::max(280.0f,
                     mat_label_w_max + st_mat.WindowPadding.x * 2.0f + st_mat.FramePadding.x * 2.0f + st_mat.ScrollbarSize + 8.0f));

  std::unordered_set<const AIS_Shape*> selected_in_viewer;
  for (const AIS_Shape_ptr& ais : m_view->get_selected())
    if (!ais.IsNull())
      selected_in_viewer.insert(ais.get());

  for (const Shp_ptr& shape : m_view->get_shapes())
  {
    const bool row_selected = selected_in_viewer.count(shape.get()) != 0;

    if (row_selected)
    {
      const ImVec4 header = ImGui::GetStyleColorVec4(ImGuiCol_Header);
      ImGui::PushStyleColor(ImGuiCol_FrameBg,
                            ImVec4(header.x, header.y, header.z, 0.40f));
      ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
                            ImVec4(header.x, header.y, header.z, 0.55f));
      ImGui::PushStyleColor(ImGuiCol_FrameBgActive,
                            ImVec4(header.x, header.y, header.z, 0.65f));
      const ImVec4 text = ImGui::GetStyleColorVec4(ImGuiCol_Text);
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(
                                               std::min(1.0f, text.x * k_shape_list_selected_text_rgb_scale + k_shape_list_selected_text_rgb_bias),
                                               std::min(1.0f, text.y * k_shape_list_selected_text_rgb_scale + k_shape_list_selected_text_rgb_bias),
                                               std::min(1.0f, text.z * k_shape_list_selected_text_rgb_scale + k_shape_list_selected_text_rgb_bias),
                                               text.w));
      ImGui::BeginGroup();
    }

    // Unique ID suffix using index
    std::string id_suffix = "##" + std::to_string(index++);
    // Editable text box for name
#pragma warning(push)
#pragma warning(disable : 4996)
    char name_buffer[1024];
    strncpy(name_buffer, shape->get_name().c_str(), sizeof(name_buffer) - 1);
    name_buffer[sizeof(name_buffer) - 1] = '\0';  // Ensure null-terminated
#pragma warning(pop)

    int mat_idx = shape->Material();
    if (mat_idx < 0 || mat_idx >= nmat)
      mat_idx = static_cast<int>(m_view->get_default_material().Name());

    auto apply_shape_material = [&](int i)
    {
      if (i < 0 || i >= nmat)
        return;

      shape->SetMaterial(Graphic3d_MaterialAspect(static_cast<Graphic3d_NameOfMaterial>(i)));
      m_view->ctx().Redisplay(shape, true);
      m_view->ctx().UpdateCurrentViewer();
    };

    ImGui::PushID(("name" + id_suffix).c_str());
    if (ImGui::InputText("", name_buffer, sizeof(name_buffer)))
      shape->set_name(std::string(name_buffer));

    if (ImGui::BeginPopupContextItem("shape_name_mat"))
    {
      if (ImGui::BeginMenu("Material"))
      {
        for (int i = 0; i < nmat; ++i)
          if (ImGui::MenuItem(mat_names[static_cast<size_t>(i)].c_str(), nullptr, i == mat_idx))
            apply_shape_material(i);
        ImGui::EndMenu();
      }
      ImGui::EndPopup();
    }
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

    ImGui::SameLine();
    ImGui::PushID(("matbtn" + id_suffix).c_str());
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2(4.0f, ImGui::GetStyle().FramePadding.y));
    if (ImGui::Button("M"))
      ImGui::OpenPopup("mat_pick");

    ImGui::PopStyleVar();
    if (m_show_tool_tips && ImGui::IsItemHovered())
      ImGui::SetTooltip("%s\n(right-click name: Material menu)", mat_names[static_cast<size_t>(mat_idx)].c_str());

    ImGui::SetNextWindowSize(ImVec2(mat_popup_w, 0.0f), ImGuiCond_Appearing);
    if (ImGui::BeginPopup("mat_pick"))
    {
      ImGui::TextUnformatted("Material");
      ImGui::Separator();
      const float max_h = ImGui::GetTextLineHeightWithSpacing() * 12.0f;
      const float sc_w  = std::max(1.0f, ImGui::GetContentRegionAvail().x);
      if (ImGui::BeginChild("mat_sc", ImVec2(sc_w, max_h), ImGuiChildFlags_Borders,
                            ImGuiWindowFlags_AlwaysVerticalScrollbar))
      {
        for (int i = 0; i < nmat; ++i)
          if (ImGui::Selectable(mat_names[static_cast<size_t>(i)].c_str(), i == mat_idx))
          {
            apply_shape_material(i);
            ImGui::CloseCurrentPopup();
          }
        ImGui::EndChild();
      }
      ImGui::EndPopup();
    }
    if (ImGui::BeginPopupContextItem("mat_btn_ctx"))
    {
      for (int i = 0; i < nmat; ++i)
        if (ImGui::MenuItem(mat_names[static_cast<size_t>(i)].c_str(), nullptr, i == mat_idx))
          apply_shape_material(i);

      ImGui::EndPopup();
    }
    ImGui::PopID();

    if (row_selected)
    {
      ImGui::EndGroup();
      ImGui::PopStyleColor(4);
      if (m_show_tool_tips && ImGui::IsItemHovered())
        ImGui::SetTooltip("Selected in 3D viewer");
    }
  }
  ImGui::End();
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
  // Undo / redo stack
  ImGui::Text("Undo: %zu (Ctrl+Z)  |  Redo: %zu (Ctrl+Y)  [max 50]",
              m_view->undo_stack_size(),
              m_view->redo_stack_size());
  ImGui::Separator();
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

// Log window implementation
void GUI::log_message(const std::string& message)
{
  if (m_log_buffer.size() > 1)
  {
    m_log_buffer.pop_back();  // trailing '\0'
    m_log_buffer.push_back('\n');
  }
  else if (!m_log_buffer.empty())
    m_log_buffer.pop_back();

  m_log_buffer.insert(m_log_buffer.end(), message.begin(), message.end());
  m_log_buffer.push_back('\0');
  m_log_scroll_to_bottom = true;
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

  if (m_log_buffer.empty())
    m_log_buffer.push_back('\0');

  ImGui::BeginChild("LogScroll", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
  const char* log_begin = m_log_buffer.data();
  const char* log_end   = m_log_buffer.size() > 1 ? log_begin + m_log_buffer.size() - 1 : log_begin;
  ImGui::TextUnformatted(log_begin, log_end);
  if (m_log_scroll_to_bottom)
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

void GUI::python_console_()
{
  if (!m_show_python_console)
    return;
  if (!m_python_console)
    m_python_console = std::make_unique<Python_console>(this);
  m_python_console->render(&m_show_python_console);
}

void GUI::init(GLFWwindow* window, ImFont* console_font)
{
  m_console_font = console_font;
  initialize_toolbar_();
  settings::set_log_callback([this](const std::string& m)
                             { log_message(m); });
  setup_log_redirection_();  // Set up stream redirection
  log_message("EzyCad: initializing 3D view...");
  m_view->init_window(window);
  m_view->init_viewer();
  m_view->init_default();
  log_message("EzyCad: 3D view ready (initial empty document).");

  load_occt_view_settings_();

  load_examples_list_();
  if (m_example_files.empty())
    log_message("EzyCad: no example projects found under res/examples.");
  else
    log_message("EzyCad: " + std::to_string(m_example_files.size()) + " example project(s) available in File > Examples.");

  load_default_project_();
}

void GUI::persist_last_opened_project_path_(const std::string& path)
{
#ifndef __EMSCRIPTEN__
  if (path.empty() || path == "(startup)")
    return;
  try
  {
    const std::filesystem::path p(path);
    if (p.empty())
      return;
    m_last_opened_project_path = p.generic_string();
    save_occt_view_settings();
  }
  catch (...)
  {
  }
#endif
}

void GUI::load_default_project_()
{
  static constexpr char k_bundled_default[] = "res/default.ezy";

  log_message("EzyCad: resolving startup document...");

#ifndef __EMSCRIPTEN__
  if (m_load_last_opened_on_startup && !m_last_opened_project_path.empty())
  {
    try
    {
      const std::filesystem::path p(m_last_opened_project_path);
      if (std::filesystem::exists(p))
      {
        std::ifstream file(p, std::ios::binary);
        if (file.is_open())
        {
          const std::string json_str {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
          if (is_valid_project_json(json_str))
          {
            log_message("EzyCad: loading last opened project: " + p.string());
            on_file(p.string(), json_str, false);
            log_message("EzyCad: startup document loaded (last opened).");
            return;
          }
          log_message("EzyCad: last opened project JSON is invalid; falling back to startup/default.");
          show_message("Last opened project file is invalid; loading startup/default.");
        }
      }
      else
        log_message("EzyCad: last opened project path not found; falling back to startup/default.");
    }
    catch (...)
    {
      log_message("EzyCad: could not load last opened project; falling back to startup/default.");
    }
  }
#endif

  const std::string user_startup = settings::load_user_startup_project();
  if (is_valid_project_json(user_startup))
  {
#ifdef __EMSCRIPTEN__
    log_message("EzyCad: loading saved startup project (browser storage).");
#else
    {
      const std::filesystem::path p = settings::user_startup_project_path();
      log_message("EzyCad: loading saved startup project: " + (p.empty() ? std::string("(user storage)") : p.string()));
    }
#endif
    on_file("(startup)", user_startup, false);
    m_last_saved_path.clear();
    log_message("EzyCad: startup document loaded (saved startup).");
    return;
  }
  if (!user_startup.empty())
  {
    log_message("EzyCad: saved startup project is invalid or incomplete; falling back to install default.");
    show_message("Saved startup project is invalid; loading install default.");
  }
  else
    log_message("EzyCad: no saved startup project; trying bundled default.");

  std::ifstream file(k_bundled_default, std::ios::binary);
  if (!file.is_open())
  {
    log_message("EzyCad: bundled " + std::string(k_bundled_default) + " not found; keeping initial empty document.");
    return;
  }
  const std::string json_str {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
  if (is_valid_project_json(json_str))
  {
    log_message("EzyCad: loading bundled default project (" + std::string(k_bundled_default) + ").");
    on_file(k_bundled_default, json_str, false);
    m_last_saved_path.clear();
    log_message("EzyCad: startup document loaded (bundled default).");
  }
  else
    log_message("EzyCad: bundled " + std::string(k_bundled_default) + " is invalid; keeping initial empty document.");
}

std::string GUI::serialized_project_json_() const
{
  using namespace nlohmann;
  std::string project_json = m_view->to_json();
  json        j            = json::parse(project_json);
  j["mode"]                = static_cast<int>(get_mode());
  return j.dump(2);
}

void GUI::save_startup_project_()
{
  if (!settings::save_user_startup_project(serialized_project_json_()))
  {
    show_message("Could not save startup project.");
    return;
  }
#ifndef __EMSCRIPTEN__
  const std::filesystem::path p = settings::user_startup_project_path();
  if (!p.empty())
    log_message("Startup saved to: " + p.string());
#endif
  show_message("Startup project saved. It will load automatically the next time you start EzyCad.");
}

void GUI::clear_saved_startup_project_()
{
  settings::clear_user_startup_project();
  show_message("Saved startup cleared. Next launch uses the install default (res/default.ezy).");
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
  const ScreenCoords screen_coords(glm::dvec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y));

  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && mods == 0)
    if (try_underlay_calib_click_(screen_coords))
    {
      m_view->on_mouse_move(screen_coords);
      m_view->ctx().UpdateCurrentViewer();
      return;
    }

  m_view->on_mouse_button(button, action, mods);

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

// Import/export related
void GUI::export_file_dialog_(Export_format fmt)
{
#ifndef __EMSCRIPTEN__
  const char* title       = "Export STEP";
  const char* def_name    = "export.step";
  const char* filter_pat  = "*.step";
  const char* filter_desc = "STEP files";
  switch (fmt)
  {
    case Export_format::Step:
      break;
    case Export_format::Iges:
      title       = "Export IGES";
      def_name    = "export.igs";
      filter_pat  = "*.igs";
      filter_desc = "IGES files";
      break;
    case Export_format::Stl:
      title       = "Export STL";
      def_name    = "export.stl";
      filter_pat  = "*.stl";
      filter_desc = "STL files";
      break;
    case Export_format::Ply:
      title       = "Export PLY";
      def_name    = "export.ply";
      filter_pat  = "*.ply";
      filter_desc = "PLY files";
      break;
  }

  char const* filter_patterns[1] = {filter_pat};
  char const* selected           = tinyfd_saveFileDialog(title, def_name, 1, filter_patterns, filter_desc);
  if (!selected)
  {
    show_message("Export canceled");
    return;
  }
  const Status s = m_view->export_document(fmt, selected);
  if (!s.is_ok())
    show_message(s.message());
  else
    show_message("Exported: " + std::filesystem::path(selected).filename().string());
#else
  const char* mem_path = "/ezycad_export.step";
  std::string download_name {"export.step"};
  switch (fmt)
  {
    case Export_format::Step:
      break;
    case Export_format::Iges:
      mem_path      = "/ezycad_export.igs";
      download_name = "export.igs";
      break;
    case Export_format::Stl:
      mem_path      = "/ezycad_export.stl";
      download_name = "export.stl";
      break;
    case Export_format::Ply:
      mem_path      = "/ezycad_export.ply";
      download_name = "export.ply";
      break;
  }
  const Status s = m_view->export_document(fmt, mem_path);
  if (!s.is_ok())
  {
    show_message(s.message());
    return;
  }
  std::ifstream in(mem_path, std::ios::binary);
  if (!in)
  {
    show_message("Export read failed.");
    return;
  }
  const std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  download_blob_async(download_name, bytes);
  show_message("Exported: " + download_name);
#endif
}

void GUI::import_file_dialog_()
{
#ifndef __EMSCRIPTEN__
  // Native: Use tinyfiledialogs
  char const* filter_patterns[3] = {"*.step", "*.stp", "*.ply"};
  char const* selected           = tinyfd_openFileDialog(
      "Import STEP or PLY",
      "",
      3,
      filter_patterns,
      "STEP / PLY files",
      0);
  if (selected)
  {
    // Binary mode required for PLY (mesh payload may contain 0x1A; Windows text mode treats that as EOF).
    std::ifstream file(selected, std::ios::binary);
    if (!file.is_open())
    {
      show_message("Error opening: " + std::filesystem::path(selected).filename().string());
      return;
    }
    const std::string file_bytes {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    if (!file_bytes.empty())
      on_import_file(selected, file_bytes);
    else
      show_message("Error opening: " + std::filesystem::path(selected).filename().string());
  }
#else
  import_file_dialog_async();
#endif
}

// Open save related
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
  open_file_dialog_async();
#endif
}

void GUI::save_file_dialog_()
{
  const std::string json_str = serialized_project_json_();

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

void GUI::on_file(const std::string& file_path, const std::string& json_str, bool announce_load)
{
  using namespace nlohmann;
  m_view->push_undo_snapshot();
  const json j = json::parse(json_str);
  m_view->load(json_str);
  m_last_saved_path = file_path;
  Mode opened_mode  = Mode::Normal;
  if (j.contains("mode") && j["mode"].is_number_integer())
  {
    const int idx = j["mode"].get<int>();
    if (idx >= 0 && idx < static_cast<int>(Mode::_count))
      opened_mode = static_cast<Mode>(idx);
  }
  set_mode(opened_mode);
#ifndef __EMSCRIPTEN__
  if (file_path != "(startup)" && !file_path.empty() && file_path != "res/default.ezy")
    persist_last_opened_project_path_(file_path);
#endif
  if (announce_load)
    show_message("Opened: " + std::filesystem::path(file_path).filename().string());
}

void GUI::on_import_file(const std::string& file_path, const std::string& file_data)
{
  std::string ext = std::filesystem::path(file_path).extension().string();
  for (char& c : ext)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

  if (ext == ".ply")
  {
    if (!m_view->import_ply(file_data))
      show_message("PLY import failed.");
    else
      show_message("Imported: " + std::filesystem::path(file_path).filename().string());
    return;
  }

  if (Status st = m_view->import_step(file_data); !st.is_ok())
    show_message(st.message());
  else
    show_message("Imported: " + std::filesystem::path(file_path).filename().string());
}

#ifdef __EMSCRIPTEN__
void GUI::open_file_dialog_async()
{
  EM_ASM({
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

void GUI::import_file_dialog_async()
{
  EM_ASM({
    var input           = document.createElement('input');
    input.type          = 'file';
    input.accept        = '.step,.stp,.ply';
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
          var length      = contents.length;
          var contentsPtr = _malloc(length);
          HEAPU8.set(contents, contentsPtr);
          Module.ccall('on_import_file_selected', null, ['string', 'number', 'number'], [fileName, contentsPtr, length]);
          _free(contentsPtr);
        };
        reader.readAsArrayBuffer(file);
      }
      document.body.removeChild(input);
    };
    input.click();
  });
}

void GUI::sketch_underlay_file_dialog_async()
{
  EM_ASM({
    var input           = document.createElement('input');
    input.type          = 'file';
    input.accept        = 'image/png,image/jpeg,image/bmp,.png,.jpg,.jpeg,.bmp';
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
          var length      = contents.length;
          var contentsPtr = _malloc(length);
          HEAPU8.set(contents, contentsPtr);
          Module.ccall('on_sketch_underlay_selected', null, ['string', 'number', 'number'], [fileName, contentsPtr, length]);
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

void GUI::download_blob_async(const std::string& default_filename, const std::string& data)
{
  EM_ASM_ARGS({
      var blob = new Blob([HEAPU8.subarray($0, $0 + $1)], { type: 'application/octet-stream' });
      var url = URL.createObjectURL(blob);
      var a = document.createElement('a');
      a.href = url;
      a.download = UTF8ToString($2);
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      URL.revokeObjectURL(url); }, data.data(), data.size(), default_filename.c_str());
}

// C-style callback for Emscripten
extern "C" void on_file_selected(const char* file_path, char* contents, int length)
{
  const std::string json_str(contents, length);
  GUI::instance().on_file(file_path, json_str);
}

extern "C" void on_import_file_selected(const char* file_path, char* contents, int length)
{
  const std::string file_bytes(contents, static_cast<size_t>(length));
  GUI::instance().on_import_file(file_path, file_bytes);
}

extern "C" void on_sketch_underlay_selected(const char* file_path, char* contents, int length)
{
  const std::string file_bytes(contents, static_cast<size_t>(length));
  GUI::instance().on_sketch_underlay_file(file_path, file_bytes);
}

extern "C" void on_save_file_selected(const char* file_name)
{
  GUI::instance().show_message(std::string("Saved: ") + file_name);
}

#endif
