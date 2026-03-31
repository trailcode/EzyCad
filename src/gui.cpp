#include "gui.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

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
#include "python_console.h"
#include "occt_view.h"
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
      {                           load_texture("User.png"),  true,                  "Inspection mode",                         Mode::Normal},
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
       "Toggle edge dimension annotation (Options: length value placement)",         Mode::Sketch_toggle_edge_dim},
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
#ifdef __EMSCRIPTEN__
    if (ImGui::MenuItem("Script console (Lua)", "Ctrl+Shift+L", m_show_lua_console))
#else
    if (ImGui::MenuItem("Script console (Lua)", "F12", m_show_lua_console))
#endif
    {
      m_show_lua_console = !m_show_lua_console;
      save_panes         = true;
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Lua scripting. Toggle with keyboard or here.");
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

  if (ImGui::IsItemDeactivatedAfterEdit() && m_dist_callback)
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

  ImGui::SetNextItemWidth(80.0f);
  ImGui::SetKeyboardFocusHere();

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
    for (const Shp_ptr& shape : m_view->get_shapes())
    {
      shape->set_visible(!m_hide_all_shapes);
    }
  }

  ImGui::Separator();

  for (const Shp_ptr& shape : m_view->get_shapes())
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

void GUI::init(GLFWwindow* window)
{
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

void GUI::load_default_project_()
{
  static constexpr char k_bundled_default[] = "res/default.ezy";

  log_message("EzyCad: resolving startup document...");

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
  // Emscripten: Call async version (synchronous fallback for simplicity)
  open_file_dialog_async("Open EzyCad project");
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
  open_file_dialog_async("Open EzyCad project");
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

extern "C" void on_save_file_selected(const char* file_name)
{
  GUI::instance().show_message(std::string("Saved: ") + file_name);
}

#endif
