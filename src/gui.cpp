#include "gui.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>

#include "utl_settings.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "third_party/tinyfiledialogs/tinyfiledialogs.h"

#include "utl_geom.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "utl_log.h"
#include "scr_lua_console.h"
#include "gui_occt_view.h"
#include "utl_io.h"
#include "scr_python_console.h"
#include "shp_info.h"
#include "sketch.h"
#include "utl.h"
#include "version.h"

#include <Standard_Version.hxx>
#include <GLFW/glfw3.h>

using namespace glm;

GUI* gui_instance = nullptr;

GUI::GUI()
{
  EZY_ASSERT(!gui_instance);

  m_view       = std::make_unique<Occt_view>(*this);
  gui_instance = this;
}

Length_dimension_style GUI::length_dimension_style() const
{
  Length_dimension_style s{};
  s.line_width        = m_edge_dim_line_width;
  s.arrow_size        = m_edge_dim_arrow_size;
  s.color_rgb[0]      = m_edge_dim_color[0];
  s.color_rgb[1]      = m_edge_dim_color[1];
  s.color_rgb[2]      = m_edge_dim_color[2];
  s.text_height_scale = m_edge_dim_text_scale;
  s.label_h           = m_edge_dim_label_h;
  s.arrow_style       = m_edge_dim_arrow_style;
  s.arrow_orientation = m_edge_dim_arrow_orientation;
  s.text_render_mode  = m_edge_dim_text_render_mode;
  return s;
}

void GUI::set_show_sketch_dimensions(const bool show)
{
  if (m_show_sketch_dimensions == show)
    return;

  m_show_sketch_dimensions = show;
  apply_sketch_dimensions_visibility();
}

void GUI::apply_sketch_dimensions_visibility()
{
  if (m_view)
    m_view->apply_sketch_dimensions_visibility();
}

ImFont* GUI::console_font() const
{
  if (!m_console_font || !ImGui::GetCurrentContext())
    return nullptr;

  ImFontAtlas* atlas = ImGui::GetIO().Fonts;
  if (!atlas)
    return nullptr;

  for (int i = 0; i < atlas->Fonts.Size; ++i)
    if (atlas->Fonts[i] == m_console_font)
      return m_console_font;

  return nullptr;
}

GUI::~GUI()
{
  cleanup_log_redirection_(); // Clean up stream redirection
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
  // Underlay transform sliders use sketch_plane_view_aabb_2d -> pt_on_plane -> view projection.
  // FlushViewEvents must run before ImGui so the camera matches the latest pan/zoom/rotate (do_frame() runs later).
  m_view->flush_view_events();

  update_window_title_();

  if (m_dark_mode)
    ImGui::StyleColorsDark();
  else
    ImGui::StyleColorsLight();

  apply_imgui_style_from_members_();

  menu_bar_();
  dock_space_();
  toolbar_();
  dist_edit_();
  angle_edit_();
  sketch_origin_set_edit_();
  sketch_list_();
  sketch_properties_dialog_();
  shape_list_();
  shape_info_dialog_();
  options_();
  message_status_window_();
  about_dialog_();
  add_box_dialog_();
  add_pyramid_dialog_();
  add_sphere_dialog_();
  add_cylinder_dialog_();
  add_cone_dialog_();
  add_torus_dialog_();
  add_sketch_dialog_();
  log_window_();
  lua_console_();
  python_console_();
  settings_();
  dbg_();
}

void GUI::render_occt() { m_view->do_frame(); }

// Initialize toolbar buttons
void GUI::initialize_toolbar_()
{
  m_toolbar_buttons = {
      // clang-format off
      {load_texture("res/icons/User.png"),                            true,  "Inspection mode",                   Mode::Normal},
      {load_texture("res/icons/Workbench_Sketcher_none.png"),         false, "Sketch inspection mode",            Mode::Sketch_inspection_mode},
      {load_texture("res/icons/Assembly_AxialMove.png"),              false, "Shape move (g)",                    Mode::Move},
      {load_texture("res/icons/Draft_Rotate.png"),                    false, "Shape rotate (r)",                  Mode::Rotate},
      {load_texture("res/icons/Part_Scale.png"),                      false, "Shape Scale (s)",                   Mode::Scale},
      {load_texture("res/icons/Macro_FaceToSketch_48.png"),           false, "Create a sketch from planar face",  Mode::Sketch_from_planar_face},
      {load_texture("res/icons/Sketcher_MirrorSketch.png"),           false, "Operational axis",                  Mode::Sketch_operation_axis},
      {load_texture("res/icons/Sketcher_CreatePoint.png"),            false, "Add node",                          Mode::Sketch_add_node},
      {load_texture("res/icons/Sketcher_Element_Line_Edge.png"),      false, "Add line edge",                     Mode::Sketch_add_edge},
      {load_texture("res/icons/ls.png"),                              false, "Add multi-line edge",               Mode::Sketch_add_multi_edges},
      {load_texture("res/icons/Sketcher_Element_Arc_Edge.png"),       false, "Add arc circle",                    Mode::Sketch_add_seg_circle_arc},
      {load_texture("res/icons/Sketcher_CreateSquare.png"),           false, "Add square",                        Mode::Sketch_add_square},
      {load_texture("res/icons/Sketcher_CreateRectangle.png"),        false, "Add rectangle from two points",     Mode::Sketch_add_rectangle},
      {load_texture("res/icons/Sketcher_CreateRectangle_Center.png"), false, "Add rectangle with center point",   Mode::Sketch_add_rectangle_center_pt},
      {load_texture("res/icons/Sketcher_CreateCircle.png"),           false, "Add circle",                        Mode::Sketch_add_circle},
      {load_texture("res/icons/Sketcher_Create3PointCircle.png"),     false, "Add circle from three points",      Mode::Sketch_add_circle_3_pts},
      {load_texture("res/icons/Sketcher_CreateSlot.png"),             false, "Add slot",                          Mode::Sketch_add_slot},
      {load_texture("res/icons/TechDraw_LengthDimension.png"),        false, "Length dimension (d)",              Mode::Sketch_dim_anno},
      {load_texture("res/icons/Design456_Extrude.png"),               false, "Extrude sketch face (e)",           Mode::Sketch_face_extrude},
      {load_texture("res/icons/PartDesign_Chamfer.png"),              false, "Chamfer (c)",                       Mode::Shape_chamfer},
      {load_texture("res/icons/PartDesign_Fillet.png"),               false, "Fillet (f)",                        Mode::Shape_fillet},
      {load_texture("res/icons/Draft_PolarArray.png"),                false, "Shape polar duplicate",             Mode::Shape_polar_duplicate},
      {load_texture("res/icons/Part_Cut.png"),                        false, "Shape cut",                         Command::Shape_cut},
      {load_texture("res/icons/Part_Fuse.png"),                       false, "Shape fuse",                        Command::Shape_fuse},
      {load_texture("res/icons/Part_Common.png"),                     false, "Shape common",                      Command::Shape_common},
      // clang-format on
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
            [](const Example_file& a, const Example_file& b) { return a.label < b.label; });
}

void GUI::seed_default_dock_layout_(ImGuiID dockspace_id)
{
  ImGui::DockBuilderRemoveNode(dockspace_id);
  // Cast private DockSpace flag: mixing ImGuiDockNodeFlagsPrivate_ with ImGuiDockNodeFlags_
  // triggers -Wdeprecated-enum-enum-conversion (e.g. emscripten/clang).
  ImGui::DockBuilderAddNode(dockspace_id, static_cast<ImGuiDockNodeFlags>(ImGuiDockNodeFlags_DockSpace) |
                                              ImGuiDockNodeFlags_PassthruCentralNode);
  ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->WorkSize);

  ImGuiID dock_main        = dockspace_id;
  ImGuiID dock_left        = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.20f, nullptr, &dock_main);
  ImGuiID dock_right       = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.22f, nullptr, &dock_main);
  ImGuiID dock_bottom      = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.12f, nullptr, &dock_main);
  ImGuiID dock_left_bottom = 0;
  ImGuiID dock_left_top    = ImGui::DockBuilderSplitNode(dock_left, ImGuiDir_Down, 0.52f, &dock_left_bottom, &dock_left);

  ImGui::DockBuilderDockWindow("Shape List", dock_left_top);
  ImGui::DockBuilderDockWindow("Sketch List", dock_left_bottom);
  ImGui::DockBuilderDockWindow("Options", dock_right);
  ImGui::DockBuilderDockWindow("Log", dock_bottom);
  ImGui::DockBuilderFinish(dockspace_id);
}

bool GUI::occt_wants_mouse_at(const float x, const float y) const
{
  if (!m_occt_passthrough_valid)
    return false;

  return x >= m_occt_passthrough_min[0] && x < m_occt_passthrough_max[0] && y >= m_occt_passthrough_min[1] &&
         y < m_occt_passthrough_max[1];
}

ScreenCoords GUI::cursor_screen_coords() const
{
  EZY_ASSERT(m_glfw_window != nullptr);
  double x = 0.0;
  double y = 0.0;
  glfwGetCursorPos(m_glfw_window, &x, &y);
  return ScreenCoords(dvec2(x, y));
}

void GUI::dock_space_()
{
  m_occt_passthrough_valid = false;

  const ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(ImGui::GetID("EzyCadMainDockSpace"), ImGui::GetMainViewport(),
                                                            ImGuiDockNodeFlags_PassthruCentralNode);

  if (m_seed_default_dock_layout)
  {
    seed_default_dock_layout_(dockspace_id);
    m_seed_default_dock_layout = false;
  }

  if (ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspace_id))
    if (ImGuiDockNode* central = node->CentralNode; central && central->IsEmpty())
    {
      m_occt_passthrough_min[0] = central->Pos.x;
      m_occt_passthrough_min[1] = central->Pos.y;
      m_occt_passthrough_max[0] = central->Pos.x + central->Size.x;
      m_occt_passthrough_max[1] = central->Pos.y + central->Size.y;
      m_occt_passthrough_valid  = true;

      const ImVec2 mouse = ImGui::GetIO().MousePos;
      if (occt_wants_mouse_at(mouse.x, mouse.y))
        ImGui::SetNextFrameWantCaptureMouse(false);
    }
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
      new_project_();

    if (ImGui::MenuItem("Open", "Ctrl+O"))
      open_file_dialog_();

    if (ImGui::MenuItem("Save", "Ctrl+S"))
      save_file_dialog_();

    if (ImGui::MenuItem("Save as"))
    {
      m_last_saved_path.clear(); // Force save as dialog
      save_file_dialog_();
    }

    if (ui_show_feature(2))
    {
      if (ImGui::MenuItem("Import"))
        import_file_dialog_();

      if (ImGui::MenuItem("Sketch underlay image..."))
      {
        m_underlay_import_sketch_target.reset();
        sketch_underlay_import_dialog_();
      }
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

    if (ui_show_feature(2) && ImGui::BeginMenu("Examples"))
    {
      for (const Example_file& ex : m_example_files)
        if (ImGui::MenuItem(ex.label.c_str()))
        {
          std::ifstream     file(ex.path, std::ios::binary);
          const std::string file_bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
          if (file.good() && !file_bytes.empty())
            on_file(ex.path, file_bytes);
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

    add_menu_items_();

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

    if (ui_show_feature(1))
    {
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
    }

    if (ui_show_feature(2))
    {
      if (ImGui::MenuItem("Lua Console", nullptr, m_show_lua_console))
      {
        m_show_lua_console = !m_show_lua_console;
        save_panes         = true;
      }

      if (ui_show_contextual_help() && ImGui::IsItemHovered())
        ImGui::SetTooltip("Interactive Lua prompt and res/scripts/lua editors.");

      if (ImGui::MenuItem("Python Console", nullptr, m_show_python_console))
      {
        m_show_python_console = !m_show_python_console;
        save_panes            = true;
      }
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
      m_open_about_popup = true;

    if (ImGui::MenuItem("Usage Guide"))
      open_url_("https://ezycad.readthedocs.io/en/latest/usage.html");

    ImGui::EndMenu();
  }

  ImGui::EndMainMenuBar();
}

void GUI::about_dialog_()
{
  if (m_open_about_popup)
  {
    m_about_popup_open = true;
    ImGui::SetNextWindowSize(ImVec2(520.0f, 520.0f), ImGuiCond_Appearing);
    ImGui::OpenPopup("About");
    m_open_about_popup = false;
  }

  if (!ImGui::BeginPopupModal("About", &m_about_popup_open, ImGuiWindowFlags_None))
    return;

  ensure_about_assets_();

  ImFont*               font = ImGui::GetFont();
  ImGui::MarkdownConfig md;

  md.linkCallback      = about_markdown_link_cb_;
  md.imageCallback     = about_markdown_image_cb_;
  md.tooltipCallback   = ImGui::defaultMarkdownTooltipCallback;
  md.linkIcon          = "";
  md.headingFormats[0] = {font, true};
  md.headingFormats[1] = {font, true};
  md.headingFormats[2] = {font, false};
  md.userData          = this;

  float const fs                = ImGui::GetFontSize();
  md.headingFormats[0].fontSize = fs * 1.15f;
  md.headingFormats[1].fontSize = fs * 1.05f;
  md.headingFormats[2].fontSize = fs;

  ImGui::BeginChild("AboutMd", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
  ImGui::Markdown(m_about_markdown.c_str(), m_about_markdown.size(), md);
  ImGui::EndChild();

  ImGui::EndPopup();
}

std::string GUI::project_title_segment_() const
{
  if (m_last_saved_path.empty() || m_last_saved_path == "(startup)")
    return "untitled";

  try
  {
    const std::string fn = std::filesystem::path(m_last_saved_path).filename().string();
    if (fn.empty())
      return "untitled";

    return fn;
  }

  catch (...)
  {
    return "untitled";
  }
}

void GUI::update_window_title_()
{
  EZY_ASSERT(m_glfw_window != nullptr);

  const std::string title = std::string("EzyCad ") + EZYCAD_VERSION_STRING + " - " + project_title_segment_();
  if (title == m_cached_window_title)
    return;

  m_cached_window_title = title;
  glfwSetWindowTitle(m_glfw_window, m_cached_window_title.c_str());
}

void GUI::open_url_(const std::string& url)
{
#ifdef __EMSCRIPTEN__
  // For Emscripten, use JavaScript's window.open()
  // Use EM_ASM for safer execution
  EM_ASM_({ window.open(UTF8ToString($0), '_blank'); }, url.c_str());
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

void GUI::doc_help_button_(const char* scope, int line, const char* tooltip, const char* doc_url, bool trailing_same_line)
{
  if (!ui_show_contextual_help())
    return;

  ImGui::PushID(scope);
  ImGui::PushID(line);
  if (ImGui::SmallButton("?"))
    if (doc_url && doc_url[0] != '\0')
      open_url_(doc_url);

  ImGui::PopID();
  ImGui::PopID();

  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("%s", tooltip);

  if (trailing_same_line)
    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
}

void GUI::ensure_about_assets_()
{
  if (m_about_assets_loaded)
    return;

  m_about_assets_loaded = true;

  const char* md_paths[] = {
#ifdef __EMSCRIPTEN__
      "/res/about.md",
#endif
      "res/about.md",
  };

  for (const char* p : md_paths)
  {
    std::ifstream f(p);
    if (!f)
      continue;

    m_about_markdown.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    break;
  }
  if (m_about_markdown.empty())
    m_about_markdown = "Could not load res/about.md.\n";
  else
    m_about_markdown = std::string("**EzyCad ") + EZYCAD_VERSION_STRING + "**\n\n" + m_about_markdown;

  m_about_markdown += "\n\nOpen CASCADE ";
  m_about_markdown += OCC_VERSION_STRING_EXT;
  m_about_markdown += "\nDear ImGui ";
  m_about_markdown += IMGUI_VERSION;
  m_about_markdown += "\nnlohmann/json ";
  m_about_markdown += std::to_string(NLOHMANN_JSON_VERSION_MAJOR);
  m_about_markdown += '.';
  m_about_markdown += std::to_string(NLOHMANN_JSON_VERSION_MINOR);
  m_about_markdown += '.';
  m_about_markdown += std::to_string(NLOHMANN_JSON_VERSION_PATCH);
  m_about_markdown += "\ntinyfiledialogs ";
  m_about_markdown += tinyfd_version;
  m_about_markdown += "\nImGuiColorTextEdit ";
  m_about_markdown += EZYCAD_IMGUI_COLOR_TEXT_EDIT_REF;
  m_about_markdown += "\n";

  const char* png_paths[] = {
#ifdef __EMSCRIPTEN__
      "/res/AI-gen-splashscreen_05_01_2026_512.png",
#endif
      "res/AI-gen-splashscreen_05_01_2026_512.png",
  };
  for (const char* p : png_paths)
  {
    if (!std::filesystem::exists(p))
      continue;

    std::ifstream fi(p, std::ios::binary);
    if (!fi)
      continue;

    std::string bytes((std::istreambuf_iterator<char>(fi)), std::istreambuf_iterator<char>());
    if (auto dec = decode_image_bytes(bytes))
    {
      m_about_splash_size.x = std::get<1>(*dec);
      m_about_splash_size.y = std::get<2>(*dec);
    }
    m_about_splash_gl = load_texture(p);
    break;
  }
}

ImGui::MarkdownImageData GUI::about_markdown_image_(ImGui::MarkdownLinkCallbackData data)
{
  ImGui::MarkdownImageData out{};
  if (!data.isImage)
    return out;

  static constexpr char k_splash_id[] = "AI-gen-splashscreen_05_01_2026_512.png";
  std::string           id(data.link, data.linkLength);
  if (id != k_splash_id || m_about_splash_gl == 0)
    return out;

  out.isValid         = true;
  out.useLinkCallback = false;
  out.user_texture_id = (ImTextureID)(intptr_t)m_about_splash_gl;
  out.size            = ImVec2((float)m_about_splash_size.x, (float)m_about_splash_size.y);
  ImVec2 const avail  = ImGui::GetContentRegionAvail();
  if (out.size.x > avail.x && avail.x > 1.0f)
  {
    float const ratio = out.size.y / out.size.x;
    out.size.x        = avail.x;
    out.size.y        = avail.x * ratio;
  }
  return out;
}

void GUI::about_markdown_link_cb_(ImGui::MarkdownLinkCallbackData data)
{
  if (data.isImage || !data.userData)
    return;

  std::string url(data.link, data.linkLength);
  static_cast<GUI*>(data.userData)->open_url_(url);
}

ImGui::MarkdownImageData GUI::about_markdown_image_cb_(ImGui::MarkdownLinkCallbackData data)
{
  if (!data.userData)
    return {};

  return static_cast<GUI*>(data.userData)->about_markdown_image_(data);
}

// Render toolbar with ImGui
void GUI::toolbar_()
{
  ImGui::Begin("Toolbar", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking);

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
    if (ImGui::ImageButton(button_id, (ImTextureID)(intptr_t)m_toolbar_buttons[i].texture_id, button_size))
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
        set_mode(std::get<Mode>(m_toolbar_buttons[i].data));
    }

    if (ui_show_help(1) && ImGui::IsItemHovered())
      ImGui::SetTooltip("%s", m_toolbar_buttons[i].tooltip);

    if (was_active)
      ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::PopID();
  }

  ImGui::End();
}

// Distance edit related
void GUI::set_dist_edit(float dist, std::function<void(float, bool)>&& callback,
                        const std::optional<ScreenCoords> screen_coords)
{
  // Sketch calls this every mousemove while TAB length mode is on; do not reset value/position each frame
  // or typed distance is replaced by the rubber-band length at the cursor.
  const bool already_editing = m_dist_callback != nullptr;
  if (!already_editing)
  {
    m_dist_val = dist;
    std::snprintf(m_dist_text_buf.data(), m_dist_text_buf.size(), "%.9g", static_cast<double>(dist));
    if (screen_coords.has_value())
      m_dist_edit_loc = *screen_coords;
    else
      m_dist_edit_loc = cursor_screen_coords();
  }

  m_dist_callback = std::move(callback);
  if (!already_editing)
    m_dist_edit_focus_pending = true;
}

void GUI::hide_dist_edit(bool apply)
{
  if (!m_dist_callback)
    return;

  float parsed{};
  if (parse_dist_text_to_float_(m_dist_text_buf.data(), parsed))
    m_dist_val = parsed;

  std::function<void(float, bool)> callback;
  std::swap(callback, m_dist_callback);
  if (apply)
    callback(m_dist_val, true);
}

void GUI::dist_edit_()
{
  if (!m_dist_callback)
    return;

  //  Set the position of the next window
  ImGui::SetNextWindowPos(ImVec2(float(m_dist_edit_loc.unsafe_get_x()), float(m_dist_edit_loc.unsafe_get_y())),
                          ImGuiCond_Always);

  // Set a small size (optional)
  ImGui::SetNextWindowSize(ImVec2(120.0f, 25.0f), ImGuiCond_Once);

  // Begin a window with minimal flags
  ImGui::Begin("FloatEdit##unique_id", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMouseInputs);

  ImGui::SetNextItemWidth(100.0f);
  // Focusing every frame prevents IsItemDeactivatedAfterEdit (click away / Tab) from ever committing.
  if (m_dist_edit_focus_pending)
  {
    ImGui::SetKeyboardFocusHere(0);
    m_dist_edit_focus_pending = false;
  }

  // Text field: InputFloat applies printf rounding so typed digits can disagree with m_dist_val.
  const bool text_changed = ImGui::InputText("##dist_edit_text", m_dist_text_buf.data(), m_dist_text_buf.size(),
                                             ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsScientific);

  if (text_changed && parse_dist_text_to_float_(m_dist_text_buf.data(), m_dist_val))
    m_dist_callback(m_dist_val, false);

  if (ImGui::IsItemDeactivatedAfterEdit() && m_dist_callback)
  {
    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
      hide_dist_edit(false);
      return;
    }

    float parsed{};
    if (parse_dist_text_to_float_(m_dist_text_buf.data(), parsed))
      m_dist_val = parsed;

    std::function<void(float, bool)> callback;
    std::swap(callback, m_dist_callback);
    callback(m_dist_val, true);
  }

  ImGui::End();
}

// Angle edit related
void GUI::set_angle_edit(float angle, std::function<void(float, bool)>&& callback,
                         const std::optional<ScreenCoords> screen_coords)
{
  const bool already_editing = m_angle_callback != nullptr;
  if (!already_editing)
  {
    m_angle_val = angle;
    std::snprintf(m_angle_text_buf.data(), m_angle_text_buf.size(), "%.9g", static_cast<double>(angle));
    if (screen_coords.has_value())
      m_angle_edit_loc = *screen_coords;
    else
      m_angle_edit_loc = cursor_screen_coords();
  }

  m_angle_callback = std::move(callback);
  if (!already_editing)
    m_angle_edit_focus_pending = true;
}

void GUI::hide_angle_edit(bool apply)
{
  if (!m_angle_callback)
    return;

  float parsed{};
  if (parse_dist_text_to_float_(m_angle_text_buf.data(), parsed))
    m_angle_val = parsed;

  std::function<void(float, bool)> callback;
  std::swap(callback, m_angle_callback);
  if (apply)
    callback(m_angle_val, true);
}

bool GUI::is_dist_or_angle_edit_active() const { return m_dist_callback != nullptr || m_angle_callback != nullptr; }

bool GUI::is_sketch_origin_set_edit_active() const { return m_sketch_origin_set_plane_idx >= 0; }

void GUI::open_sketch_origin_set_edit_(const Sketch::sptr& sk, int plane_idx, double v_min, double v_max)
{
  EZY_ASSERT(sk);
  EZY_ASSERT(plane_idx == 0 || plane_idx == 1);

  m_sketch_origin_set_sketch        = sk;
  m_sketch_origin_set_plane_idx     = plane_idx;
  m_sketch_origin_set_v_min         = v_min;
  m_sketch_origin_set_v_max         = v_max;
  m_sketch_origin_set_loc           = ImGui::GetItemRectMin();
  m_sketch_origin_set_focus_pending = true;
  std::snprintf(m_sketch_origin_set_text_buf.data(), m_sketch_origin_set_text_buf.size(), "%.9g",
                m_sketch_origin_xy[plane_idx]);
}

void GUI::hide_sketch_origin_set_edit(bool apply)
{
  if (m_sketch_origin_set_plane_idx < 0)
    return;

  const Sketch::sptr sk         = m_sketch_origin_set_sketch.lock();
  const int          plane_idx  = m_sketch_origin_set_plane_idx;
  const double       v_min      = m_sketch_origin_set_v_min;
  const double       v_max      = m_sketch_origin_set_v_max;
  m_sketch_origin_set_plane_idx = -1;
  m_sketch_origin_set_sketch.reset();

  if (!sk || !apply)
    return;

  float parsed{};
  if (!parse_dist_text_to_float_(m_sketch_origin_set_text_buf.data(), parsed))
    return;

  m_sketch_origin_xy[plane_idx] = std::clamp(static_cast<double>(parsed), v_min, v_max);
  sk->set_origin_pt(gp_Pnt2d(m_sketch_origin_xy[0], m_sketch_origin_xy[1]));
  m_view->ctx().UpdateCurrentViewer();
}

void GUI::sketch_origin_set_edit_()
{
  if (m_sketch_origin_set_plane_idx < 0)
    return;

  if (!m_sketch_origin_set_sketch.lock())
  {
    m_sketch_origin_set_plane_idx = -1;
    return;
  }

  ImGui::SetNextWindowPos(m_sketch_origin_set_loc, ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(120.0f, 25.0f), ImGuiCond_Once);
  ImGui::Begin("OriginSetEdit##unique_id", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoSavedSettings);

  ImGui::SetNextItemWidth(100.0f);
  if (m_sketch_origin_set_focus_pending)
  {
    ImGui::SetKeyboardFocusHere(0);
    m_sketch_origin_set_focus_pending = false;
  }

  ImGui::InputText("##origin_set_text", m_sketch_origin_set_text_buf.data(), m_sketch_origin_set_text_buf.size(),
                   ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsScientific);

  if (ImGui::IsItemDeactivatedAfterEdit())
    hide_sketch_origin_set_edit(true);

  ImGui::End();
}

void GUI::angle_edit_()
{
  if (!m_angle_callback)
    return;

  //  Set the position of the next window
  ImGui::SetNextWindowPos(ImVec2(float(m_angle_edit_loc.unsafe_get_x()), float(m_angle_edit_loc.unsafe_get_y())),
                          ImGuiCond_Always);

  // Set a small size (optional)
  ImGui::SetNextWindowSize(ImVec2(120.0f, 25.0f), ImGuiCond_Once);

  // Begin a window with minimal flags
  ImGui::Begin("AngleEdit##unique_id", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMouseInputs);

  ImGui::SetNextItemWidth(100.0f);
  if (m_angle_edit_focus_pending)
  {
    ImGui::SetKeyboardFocusHere(0);
    m_angle_edit_focus_pending = false;
  }

  const bool text_changed = ImGui::InputText("##angle_edit_text", m_angle_text_buf.data(), m_angle_text_buf.size(),
                                             ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsScientific);

  if (text_changed && parse_dist_text_to_float_(m_angle_text_buf.data(), m_angle_val))
    m_angle_callback(m_angle_val, false);

  if (ImGui::IsItemDeactivatedAfterEdit() && m_angle_callback)
  {
    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
      hide_angle_edit(false);
      return;
    }

    float parsed{};
    if (parse_dist_text_to_float_(m_angle_text_buf.data(), parsed))
      m_angle_val = parsed;

    std::function<void(float, bool)> callback;
    std::swap(callback, m_angle_callback);
    callback(m_angle_val, true);
  }

  ImGui::End();
}

bool GUI::parse_dist_text_to_float_(const char* buf, float& out)
{
  if (!buf)
    return false;

  const char* s = buf;
  while (*s != '\0' && std::isspace(static_cast<unsigned char>(*s)))
    ++s;
  if (*s == '\0')
    return false;

  char*       end = nullptr;
  const float v   = std::strtof(s, &end);
  if (end == s)
    return false;

  while (*end != '\0' && std::isspace(static_cast<unsigned char>(*end)))
    ++end;
  if (*end != '\0')
    return false;

  out = v;
  return true;
}

bool GUI::is_valid_project_manifest_(const std::string& s)
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

bool GUI::is_valid_project_file_(const std::string& bytes)
{
  if (is_ezy_zip(bytes))
  {
    const auto unpacked = unpack_ezy(bytes);
    return unpacked && is_valid_project_manifest_(unpacked->manifest_json);
  }

  return is_ezy_json(bytes) && is_valid_project_manifest_(bytes);
}

std::optional<std::string> GUI::manifest_from_project_file_(const std::string& file_bytes, Occt_view& view, bool replace_assets)
{
  if (is_ezy_zip(file_bytes))
  {
    auto unpacked = unpack_ezy(file_bytes);
    if (!unpacked)
      return std::nullopt;

    if (replace_assets)
      view.asset_store().clear();

    for (auto& [id, data] : unpacked->assets)
      view.asset_store().import_asset(id, std::move(data));

    return unpacked->manifest_json;
  }

  if (is_ezy_json(file_bytes))
    return file_bytes;

  return std::nullopt;
}

const std::vector<std::string>& GUI::occt_material_combo_labels_()
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

void GUI::sketch_list_inspector_(const Sketch::sptr& sketch, int index, Sketch::sptr& hover_sketch, size_t& hover_dim_index)
{
  ImGui::Indent();
  ImGui::PushID(index);

  const auto draw_section = [](const char* title, const std::vector<std::string>& labels)
  {
    const size_t       count = labels.size();
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
    if (count == 0)
      flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (ImGui::TreeNodeEx(title, flags, "%s (%zu)", title, count))
    {
      for (const std::string& label : labels)
        ImGui::BulletText("%s", label.c_str());

      if (count > 0)
        ImGui::TreePop();
    }
  };

  {
    const std::vector<std::string> labels = sketch->inspector_dimension_labels();
    const size_t                   count  = labels.size();
    ImGuiTreeNodeFlags             flags  = ImGuiTreeNodeFlags_SpanAvailWidth;
    if (count == 0)
      flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (ImGui::TreeNodeEx("Dimensions", flags, "Dimensions (%zu)", count))
    {
      if (count > 0 && ImGui::BeginTable("sketch_dim_rows", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp))
      {
        ImGui::TableSetupColumn("show", ImGuiTableColumnFlags_WidthFixed, 28.f);
        ImGui::TableSetupColumn("dim", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("offset", ImGuiTableColumnFlags_WidthFixed, 120.f);

        for (size_t i = 0; i < count; ++i)
        {
          bool   visible     = sketch->dimension_visible(i);
          double offset      = sketch->dimension_offset(i);
          bool   row_hovered = false;

          ImGui::PushID(static_cast<int>(i));
          ImGui::TableNextRow();

          ImGui::TableSetColumnIndex(0);
          if (ImGui::Checkbox("##dim_visible", &visible))
            sketch->set_dimension_visible(i, visible);

          row_hovered |= ImGui::IsItemHovered();

          ImGui::TableSetColumnIndex(1);
          char        name_buf[128];
          std::string cur_name = sketch->dimension_name(i);
          safe_cstr_copy(name_buf, sizeof(name_buf), cur_name.c_str());
          ImGui::SetNextItemWidth(-FLT_MIN);
          if (ImGui::InputText("##dim_name", name_buf, sizeof(name_buf)))
            sketch->set_dimension_name(i, std::string(name_buf));

          row_hovered |= ImGui::IsItemHovered();

          ImGui::TableSetColumnIndex(2);
          ImGui::SetNextItemWidth(86.f);
          if (ImGui::InputDouble("##dim_offset", &offset, 0.5, 2.0, "%.2f"))
            sketch->set_dimension_offset(i, offset);

          row_hovered |= ImGui::IsItemHovered();

          if (ui_show_contextual_help() && ImGui::IsItemHovered())
            ImGui::SetTooltip("Label offset from edge. 0 = automatic.");

          if (row_hovered && sketch->is_visible() && sketch->shows_dimensions() && visible &&
              !sketch->length_dimension_handle(i).IsNull())
          {
            hover_sketch    = sketch;
            hover_dim_index = i;
          }

          ImGui::PopID();
        }

        ImGui::EndTable();
      }

      if (count > 0)
        ImGui::TreePop();
    }
  }

  draw_section("Nodes", sketch->inspector_node_labels());
  draw_section("Edges", sketch->inspector_edge_labels());
  draw_section("Faces", sketch->inspector_face_labels());

  ImGui::PopID();
  ImGui::Unindent();
}

void GUI::sketch_list_()
{
  if (!show_sketch_list_effective())
  {
    m_view->set_sketch_list_hover(nullptr);
    m_view->set_sketch_list_measurement_hover(nullptr, SIZE_MAX);
    return;
  }

  const ImGuiStyle& st              = ImGui::GetStyle();
  float             max_name_text_w = 0.f;
  for (const Sketch::sptr& s : m_view->get_sketches())
  {
    EZY_ASSERT(s);
    const std::string& nm = s->get_name();
    max_name_text_w       = std::max(max_name_text_w, ImGui::CalcTextSize(nm.c_str(), nm.c_str() + nm.size()).x);
  }

  if (!ImGui::Begin("Sketch List", &m_show_sketch_list, ImGuiWindowFlags_None))
  {
    m_view->set_sketch_list_hover(nullptr);
    m_view->set_sketch_list_measurement_hover(nullptr, SIZE_MAX);
    ImGui::End();
    return;
  }

  ImGui::BeginChild("##sketch_list_scroll", ImVec2(0.f, 0.f), false, ImGuiWindowFlags_HorizontalScrollbar);
  const float name_field_w = list_name_field_width_(st, max_name_text_w);

  int          index = 0;
  Sketch::sptr sketch_to_delete;
  Sketch::sptr sketch_list_hover;
  Sketch::sptr sketch_list_measurement_hover_sketch;
  size_t       sketch_list_measurement_hover_index = SIZE_MAX;
  for (Sketch::sptr& sketch : m_view->get_sketches())
  {
    EZY_ASSERT(sketch);

    ImGui::BeginGroup();

    // Buffer for editable name
    char name_buffer[1024];
    safe_cstr_copy(name_buffer, sizeof(name_buffer), sketch->get_name().c_str());

    // Unique ID suffix using index
    std::string id_suffix = "##" + std::to_string(index);

    const Sketch* sk_key   = sketch.get();
    bool&         expanded = m_sketch_list_expanded[sk_key];

    if (ui_show_sketch_list_expand())
    {
      ImGui::PushID(("expand" + id_suffix).c_str());
      if (ImGui::SmallButton(expanded ? "v" : ">"))
        expanded = !expanded;

      if (ui_show_contextual_help() && ImGui::IsItemHovered())
        ImGui::SetTooltip(expanded ? "Collapse details" : "Expand details");

      ImGui::PopID();

      ImGui::SameLine();
    }

    // Radio button for selection
    ImGui::PushID(("select" + id_suffix).c_str());
    if (ImGui::RadioButton("", &m_view->curr_sketch() == sketch.get()))
    {
      m_view->set_curr_sketch(sketch);
      if (!is_sketch_mode(get_mode()))
        set_mode(Mode::Sketch_inspection_mode);
    }

    if (ui_show_contextual_help() && ImGui::IsItemHovered())
      ImGui::SetTooltip("Sets current");

    ImGui::PopID();

    // Text edit for name (width fits longest sketch name across the list, capped)
    ImGui::SameLine();
    ImGui::PushID(("name" + id_suffix).c_str());
    ImGui::SetNextItemWidth(name_field_w);
    if (ImGui::InputText("", name_buffer, sizeof(name_buffer)))
      sketch->set_name(std::string(name_buffer));

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
    {
      if (!visible && m_view->sketch_list_hover() == sketch)
        m_view->set_sketch_list_hover(nullptr);

      sketch->set_visible(visible);
    }

    if (ui_show_contextual_help() && ImGui::IsItemHovered())
      ImGui::SetTooltip("Visibility");

    ImGui::PopID();

    ImGui::SameLine();
    ImGui::PushID(("uldisp" + id_suffix).c_str());
    {
      const bool has_ul = sketch->underlay().has_image();
      bool       dummy_off{false};
      bool       ul_vis = has_ul && sketch->underlay().visible();
      if (!has_ul)
        ImGui::BeginDisabled();

      bool* const vptr = has_ul ? &ul_vis : &dummy_off;
      if (ImGui::Checkbox("", vptr) && has_ul)
      {
        sketch->underlay().set_visible_sync(ul_vis, sketch->get_plane());
        if (m_underlay_panel_sketch == sketch.get())
          m_underlay_vis = ul_vis;
        if (m_view->sketch_list_hover() == sketch)
        {
          m_view->set_sketch_list_hover(nullptr);
          m_view->set_sketch_list_hover(sketch);
        }
      }

      if (!has_ul)
        ImGui::EndDisabled();

      if (ui_show_contextual_help() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip(has_ul ? "Display underlay" : "Import an image in Sketch properties to enable the underlay.");
    }
    ImGui::PopID();

    if (ui_show_sketch_list_props_slot())
    {
      ImGui::SameLine();
      ImGui::PushID(("props" + id_suffix).c_str());
      if (ui_show_sketch_list_props_button())
      {
        if (ImGui::SmallButton("[P]"))
        {
          m_view->set_curr_sketch(sketch);
          if (!is_sketch_mode(get_mode()))
            set_mode(Mode::Sketch_inspection_mode);
          m_sketch_properties_sketch = sketch;
          m_sketch_properties_open   = true;
        }

        if (ui_show_contextual_help() && ImGui::IsItemHovered())
          ImGui::SetTooltip("Sketch properties");
      }
      else
      {
        ImGui::BeginDisabled();
        ImGui::SmallButton("[P]");
        ImGui::EndDisabled();
        if (ui_show_contextual_help() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
          ImGui::SetTooltip("Sketch properties ([P]) unlocks at UI verbosity 3.");
      }

      ImGui::PopID();
    }

    ImGui::EndGroup();

    if (ImGui::IsItemHovered() && sketch->is_visible())
      sketch_list_hover = sketch;

    if (expanded && ui_show_sketch_list_expand())
      sketch_list_inspector_(sketch, index, sketch_list_measurement_hover_sketch, sketch_list_measurement_hover_index);

    ++index;
  }

  m_view->set_sketch_list_hover(sketch_list_hover);
  m_view->set_sketch_list_measurement_hover(sketch_list_measurement_hover_sketch, sketch_list_measurement_hover_index);

  ImGui::EndChild();

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
  char const* selected = tinyfd_openFileDialog("Sketch underlay image", "", 4, filter_patterns, "PNG / JPEG / BMP", 0);
  if (selected)
  {
    std::ifstream file(selected, std::ios::binary);
    if (!file.is_open())
    {
      show_message("Error opening: " + std::filesystem::path(selected).filename().string());
      return;
    }

    const std::string file_bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    if (!file_bytes.empty())
      on_sketch_underlay_file(selected, file_bytes);
    else
      show_message("Error opening: " + std::filesystem::path(selected).filename().string());
  }
#else
  sketch_underlay_file_dialog_async();
#endif
}

void GUI::sketch_properties_dialog_()
{
  if (!m_sketch_properties_open)
  {
    hide_sketch_origin_set_edit(false);
    return;
  }

  const Sketch::sptr sk = m_view->curr_sketch_shared();
  if (!sk)
  {
    m_sketch_properties_open = false;
    hide_sketch_origin_set_edit(false);
    return;
  }

  m_sketch_properties_sketch = sk;

  ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_FirstUseEver);
  bool open = m_sketch_properties_open;
  if (!ImGui::Begin("Sketch properties", &open, ImGuiWindowFlags_None))
  {
    m_sketch_properties_open = open;
    if (!open)
      hide_sketch_origin_set_edit(false);
    ImGui::End();
    return;
  }

  ImGui::TextUnformatted(sk->get_name().c_str());
  ImGui::Separator();
  sketch_origin_panel_settings_(sk);
  sketch_underlay_panel_settings_(sk);
  m_sketch_properties_open = open;
  ImGui::End();
}

void GUI::sketch_origin_panel_settings_(const Sketch::sptr& sk)
{
  EZY_ASSERT(sk);

  if (m_sketch_origin_panel_sketch != sk.get())
  {
    hide_sketch_origin_set_edit(false);
    m_sketch_origin_panel_sketch = sk.get();
    const gp_Pnt2d o             = sk->origin_pt();
    m_sketch_origin_xy[0]        = o.X();
    m_sketch_origin_xy[1]        = o.Y();
  }

  ImGui::TextUnformatted("Origin");
  ImGui::Separator();

  bool show_origin = sk->show_origin_marker();
  if (ImGui::Checkbox("Show origin marker", &show_origin))
  {
    sk->set_show_origin_marker(show_origin);
    m_view->ctx().UpdateCurrentViewer();
  }

  ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
  GUI_DOC_HELP_("When off, hides the Origin +/circle for this sketch (active sketch only) and removes it from snap targets.",
                doc_urls::k_sketch_origin);

  const ImGuiIO& io    = ImGui::GetIO();
  double         min_u = 0., min_v = 0., max_u = 1., max_v = 1.;
  const bool     have_view = m_view->sketch_plane_view_aabb_2d(sk->get_plane(), static_cast<double>(io.DisplaySize.x),
                                                               static_cast<double>(io.DisplaySize.y), min_u, min_v, max_u, max_v);
  if (!have_view)
  {
    constexpr double k_fallback = 250.0;
    min_u                       = m_sketch_origin_xy[0] - k_fallback;
    max_u                       = m_sketch_origin_xy[0] + k_fallback;
    min_v                       = m_sketch_origin_xy[1] - k_fallback;
    max_v                       = m_sketch_origin_xy[1] + k_fallback;
  }

  auto apply_origin = [&]()
  {
    sk->set_origin_pt(gp_Pnt2d(m_sketch_origin_xy[0], m_sketch_origin_xy[1]));
    m_view->ctx().UpdateCurrentViewer();
  };

  auto origin_axis_row = [&](const char* axis_label, const char* popup_id, int plane_idx, double v_min, double v_max)
  {
    ImGui::PushID(popup_id);
    double* const p_value = &m_sketch_origin_xy[plane_idx];
    const bool    changed =
        ImGui::SliderScalar(axis_label, ImGuiDataType_Double, p_value, &v_min, &v_max, "%.4f", ImGuiSliderFlags_ClampOnInput);

    if (changed)
      apply_origin();

    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
    if (ImGui::SmallButton("Set"))
      open_sketch_origin_set_edit_(sk, plane_idx, v_min, v_max);

    ImGui::PopID();
  };

  ImGui::TextUnformatted("Position (sketch plane, vs. current view)");
  ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
  GUI_DOC_HELP_("Sketch plane coordinates for the permanent Origin marker (+ with circle). Click ? for the user guide.",
                doc_urls::k_sketch_origin);

  ImGui::PushID(sk.get());
  // Labels match typical sketch axes on screen: "X" drives plane Y, "Y" plane X (same as underlay center).
  origin_axis_row("X", "sketch_origin_set_x", 1, min_v, max_v);
  origin_axis_row("Y", "sketch_origin_set_y", 0, min_u, max_u);
  ImGui::PopID();

  ImGui::Separator();
}

void GUI::sketch_underlay_panel_settings_(const Sketch::sptr& sk)
{
  EZY_ASSERT(sk);

  Sketch_underlay& ul              = sk->underlay();
  const gp_Pln&    ul_pln          = sk->get_plane();
  const bool       ul_sketch_shown = sk->is_visible();

  ImGui::TextUnformatted("Image underlay");
  ImGui::Separator();

  if (m_underlay_panel_sketch != sk.get())
  {
    m_underlay_panel_sketch = sk.get();
    if (ul.has_image())
    {
      ul.ui_params(m_underlay_center.x, m_underlay_center.y, m_underlay_half_extents.x, m_underlay_half_extents.y,
                   m_underlay_rot);
      // Also seed raw 6-DOF for sheared editor (will be refreshed live in the transform section too).
      {
        gp_Pnt2d b;
        gp_Vec2d au, av;
        ul.get_affine(b, au, av);
        m_underlay_base = dvec2(b.X(), b.Y());
        m_underlay_u    = dvec2(au.X(), au.Y());
        m_underlay_v    = dvec2(av.X(), av.Y());
      }
      m_underlay_raw_shear    = ul.raw_shear_display();
      m_underlay_flip_u       = ul.flip_image_u();
      m_underlay_flip_v       = ul.flip_image_v();
      m_underlay_calib_x_done = false;
      m_underlay_calib_y_done = false;
      m_underlay_opacity      = ul.opacity();
      m_underlay_vis          = ul.visible();
      m_underlay_key_white    = ul.key_white_transparent();
      m_underlay_line_tint    = ul.line_tint_enabled();
      {
        uint8_t tr, tg, tb, ta;
        ul.line_tint_rgba(tr, tg, tb, ta);
        m_underlay_tint_col[0] = static_cast<float>(tr) / 255.f;
        m_underlay_tint_col[1] = static_cast<float>(tg) / 255.f;
        m_underlay_tint_col[2] = static_cast<float>(tb) / 255.f;
        m_underlay_tint_col[3] = static_cast<float>(ta) / 255.f;
      }
    }
    else
    {
      m_underlay_center       = glm::dvec2(0.0, 0.0);
      m_underlay_half_extents = glm::dvec2(0.0, 0.0);
      m_underlay_rot          = 0.0;
      m_underlay_raw_shear    = false;
      m_underlay_flip_u       = false;
      m_underlay_flip_v       = false;
      m_underlay_calib_x_done = false;
      m_underlay_calib_y_done = false;
      m_underlay_opacity      = 0.88f;
      m_underlay_vis          = true;
      m_underlay_key_white    = true;
      m_underlay_line_tint    = true;
      m_underlay_tint_col     = glm::vec4(1.f, 220.f / 255.f, 0.f, 1.f);
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
    if (ul.has_image())
    {
      m_view->push_undo_snapshot();
      ul.clear_and_update();
      m_underlay_panel_sketch = nullptr;
    }

  ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
  GUI_DOC_HELP_("Import PNG/JPEG/BMP as a sketch underlay. Adjust half-width, half-height, center, and rotation to match "
                "real dimensions; changes apply in real time. Click ? to open the user guide.",
                doc_urls::k_image_underlay);

  if (!ul.has_image())
    return;

  if (ImGui::Checkbox("White paper -> transparent", &m_underlay_key_white))
  {
    ul.set_key_white_transparent(m_underlay_key_white);
    ul.rebuild_display(ul_pln, ul_sketch_shown);
  }

  if (ui_show_contextual_help() && ImGui::IsItemHovered())
    ImGui::SetTooltip("Uses brightness: white background becomes clear; dark lines stay visible. "
                      "Turn off for full-color photos. Inverting the image is not needed for typical scans.");

  if (ImGui::Checkbox("Tint visible lines", &m_underlay_line_tint))
  {
    ul.set_line_tint_enabled(m_underlay_line_tint);
    ul.rebuild_display(ul_pln, ul_sketch_shown);
  }

  if (ui_show_contextual_help() && ImGui::IsItemHovered())
    ImGui::SetTooltip("Paints non-transparent pixels (after white key) with the line color. "
                      "Default amber/gold contrasts with the dark view and cyan frame.");

  if (m_underlay_line_tint)
    if (ImGui::ColorEdit4("Line color", &m_underlay_tint_col[0]))
    {
      const auto to_u8 = [](float c) -> uint8_t
      {
        const float x = std::clamp(c, 0.f, 1.f) * 255.f;
        return static_cast<uint8_t>(x + 0.5f);
      };
      ul.set_line_tint_rgba(to_u8(m_underlay_tint_col[0]), to_u8(m_underlay_tint_col[1]), to_u8(m_underlay_tint_col[2]),
                            to_u8(m_underlay_tint_col[3]));
      ul.rebuild_display(ul_pln, ul_sketch_shown);
    }

  if (ImGui::SliderFloat("Opacity", &m_underlay_opacity, 0.f, 1.f, "%.2f"))
    ul.set_opacity_live(m_underlay_opacity);

  if (ui_show_contextual_help() && ImGui::IsItemHovered())
    ImGui::SetTooltip("Overall opacity of the underlay image (0 = fully transparent, 1 = fully opaque).");

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
        hint = "First click: bitmap corner (0,0) in the current underlay transform (sliders / rotation).";
        break;
      case Underlay_calib_phase::PickX2:
        hint = "Second click: along bitmap +U from that corner; calibration refines scale from this placement.";
        break;
      case Underlay_calib_phase::AwaitDistX:
        hint = "Enter the drawing distance for X (dimension popup). If Y is not calibrated yet, its scale still follows "
               "image aspect.";
        break;
      case Underlay_calib_phase::PickY1:
        hint = "First click: along bitmap height (+V) using the current underlay transform.";
        break;
      case Underlay_calib_phase::PickY2:
        hint = "Second click: farther along +V; calibration refines Y from this placement.";
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
    if (ImGui::Button("Set X from edge..."))
      begin_underlay_calib_set_x_(sk);

    ImGui::EndDisabled();
    if (ui_show_contextual_help() && ImGui::IsItemHovered())
      ImGui::SetTooltip("Two clicks along width (+U), then type the real drawing distance (same units as sketch dimensions). "
                        "You can use Set Y before or after; until both are set, the other axis still follows default aspect.");

    ImGui::SameLine();
    ImGui::BeginDisabled(!sk_is_cur || pick_x);
    if (ImGui::Button("Set Y from edge..."))
      begin_underlay_calib_set_y_(sk);

    ImGui::EndDisabled();
    if (ui_show_contextual_help() && ImGui::IsItemHovered())
      ImGui::SetTooltip(
          "Two clicks along height (+V), then enter the drawing distance for Y. Order vs. Set X does not matter.");
  }

  ImGui::Separator();
  ImGui::TextUnformatted("Transform (sketch plane, vs. current view)");
  {
    // Refresh raw affine every time this pane renders for the active underlay sketch.
    // This keeps the 6-DOF editor in sync after calibration, external changes, or when an edit
    // causes the basis to become (or stop being) orthogonal.
    gp_Pnt2d b;
    gp_Vec2d au, av;
    ul.get_affine(b, au, av);
    m_underlay_base = dvec2(b.X(), b.Y());
    m_underlay_u    = dvec2(au.X(), au.Y());
    m_underlay_v    = dvec2(av.X(), av.Y());

    const bool ul_ortho = ul.axes_orthogonal();

    if (!ul_ortho)
    {
      ImGui::TextWrapped(
          "Edge calibration produced shear (bitmap U and V are not perpendicular). The fields below directly edit the "
          "raw affine basis (origin + U and V vectors). The orthogonal sliders are unavailable so they cannot overwrite "
          "the calibrated fit. You can still use the calibration buttons or the Make orthogonal button below.");
    }

    if (ul_ortho)
    {
      // Orthogonal case: original friendly Center / Half extents / Rotation controls (unchanged behavior).
      const ImGuiIO& io    = ImGui::GetIO();
      double         min_u = 0., min_v = 0., max_u = 1., max_v = 1.;
      const bool     have_view =
          m_view->sketch_plane_view_aabb_2d(sk->get_plane(), static_cast<double>(io.DisplaySize.x),
                                            static_cast<double>(io.DisplaySize.y), min_u, min_v, max_u, max_v);
      if (!have_view)
      {
        constexpr double k_fallback = 250.0;
        // m_underlay_center stays sketch-plane gp_Pnt2d X / Y (same as underlay_ui_params).
        min_u = m_underlay_center.x - k_fallback;
        max_u = m_underlay_center.x + k_fallback;
        min_v = m_underlay_center.y - k_fallback;
        max_v = m_underlay_center.y + k_fallback;
      }

      const double span_u      = std::max(max_u - min_u, 1e-6);
      const double span_v      = std::max(max_v - min_v, 1e-6);
      const double half_span_u = 0.5 * span_u;
      const double half_span_v = 0.5 * span_v;
      // Allow underlay larger than the visible frustum (trace paper / zoomed views).
      const double     max_half_w = std::max(std::hypot(half_span_u, half_span_v) * 2.5, 1e-3);
      const double     max_half_h = max_half_w;
      constexpr double k_min_half = 1e-6;
      double           rot_min    = -180.0;
      double           rot_max    = 180.0;

      auto apply_ul_xform = [&]()
      {
        if (!ul.axes_orthogonal())
          return;

        ul.set_center_extents_rotation_display(dvec2(m_underlay_center.x, m_underlay_center.y),
                                               dvec2(m_underlay_half_extents.x, m_underlay_half_extents.y), m_underlay_rot,
                                               ul_pln, ul_sketch_shown);
      };

      auto transform_slider = [&](const char* label, ImGuiDataType type, void* p_data, const void* p_min, const void* p_max,
                                  const char* format, ImGuiSliderFlags flags = 0)
      {
        const bool changed = ImGui::SliderScalar(label, type, p_data, p_min, p_max, format, flags);
        if (ImGui::IsItemActivated())
          m_view->push_undo_snapshot();

        if (changed)
          apply_ul_xform();
      };

      auto transform_input_double = [&](const char* label, double* p_data, double v_min, double v_max, const char* format)
      {
        const bool changed = ImGui::InputDouble(label, p_data, 0.0, 0.0, format);
        if (ImGui::IsItemActivated())
          m_view->push_undo_snapshot();

        if (changed)
        {
          *p_data = std::clamp(*p_data, v_min, v_max);
          apply_ul_xform();
        }
      };

      // Labels match typical sketch axes on screen: "Center X" drives plane Y (gp_Pnt2d::Y / view v), "Center Y" plane X (u).
      transform_slider("Center X", ImGuiDataType_Double, &m_underlay_center.y, &min_v, &max_v, "%.4f",
                       ImGuiSliderFlags_ClampOnInput);
      transform_slider("Center Y", ImGuiDataType_Double, &m_underlay_center.x, &min_u, &max_u, "%.4f",
                       ImGuiSliderFlags_ClampOnInput);
      transform_slider("Half width", ImGuiDataType_Double, &m_underlay_half_extents.x, &k_min_half, &max_half_w, "%.4f",
                       ImGuiSliderFlags_ClampOnInput | ImGuiSliderFlags_Logarithmic);
      transform_slider("Half height", ImGuiDataType_Double, &m_underlay_half_extents.y, &k_min_half, &max_half_h, "%.4f",
                       ImGuiSliderFlags_ClampOnInput | ImGuiSliderFlags_Logarithmic);
      transform_slider("Rotation (deg)", ImGuiDataType_Double, &m_underlay_rot, &rot_min, &rot_max, "%.1f",
                       ImGuiSliderFlags_ClampOnInput);
      transform_input_double("Rotation value (deg)", &m_underlay_rot, rot_min, rot_max, "%.4f");
    }
    else
    {
      // Sheared (non-orthogonal) case: 6-DOF raw affine editor.
      // Base = bitmap (0,0) location in sketch plane.
      // U and V are the full edge vectors (their magnitudes are the half-extents in those directions).
      // Base X/Y use viewport-relative sliders (matching the previous Center X/Y behavior for position).

      m_underlay_raw_shear = ul.raw_shear_display();
      if (ImGui::Checkbox("Show raw shear distortion in image (makes bad Y-axis or other calibration mistakes visible as skew "
                          "in the raster)",
                          &m_underlay_raw_shear))
      {
        ul.set_raw_shear_display(m_underlay_raw_shear);
        ul.rebuild_display(ul_pln, ul_sketch_shown);
      }

      if (m_underlay_raw_shear)
      {
        m_underlay_flip_u = ul.flip_image_u();
        m_underlay_flip_v = ul.flip_image_v();

        if (ImGui::Checkbox("Reverse image U (flip horizontal in source)", &m_underlay_flip_u))
        {
          ul.set_flip_image_u(m_underlay_flip_u);
          ul.rebuild_display(ul_pln, ul_sketch_shown);
        }
        if (ImGui::Checkbox("Reverse image V (flip vertical in source)", &m_underlay_flip_v))
        {
          ul.set_flip_image_v(m_underlay_flip_v);
          ul.rebuild_display(ul_pln, ul_sketch_shown);
        }
        ImGui::TextDisabled("(Cycle these to put raster (0,0) at the 'top left' corner of the parallelogram)");
      }

      const ImGuiIO& io    = ImGui::GetIO();
      double         min_u = 0., min_v = 0., max_u = 1., max_v = 1.;
      const bool     have_view =
          m_view->sketch_plane_view_aabb_2d(sk->get_plane(), static_cast<double>(io.DisplaySize.x),
                                            static_cast<double>(io.DisplaySize.y), min_u, min_v, max_u, max_v);
      if (!have_view)
      {
        constexpr double k_fallback = 250.0;
        min_u                       = m_underlay_base.x - k_fallback;
        max_u                       = m_underlay_base.x + k_fallback;
        min_v                       = m_underlay_base.y - k_fallback;
        max_v                       = m_underlay_base.y + k_fallback;
      }

      auto apply_affine = [&]()
      {
        sk->underlay().set_affine_plane(gp_Pnt2d(m_underlay_base.x, m_underlay_base.y),
                                        gp_Vec2d(m_underlay_u.x, m_underlay_u.y), gp_Vec2d(m_underlay_v.x, m_underlay_v.y),
                                        sk->get_plane(), sk->is_visible());
      };

      // Base position sliders (viewport frustum bounds + fallback, direct plane X/Y).
      {
        const bool changed = ImGui::SliderScalar("Base X", ImGuiDataType_Double, &m_underlay_base.x, &min_u, &max_u, "%.4f",
                                                 ImGuiSliderFlags_ClampOnInput);
        if (ImGui::IsItemActivated())
          m_view->push_undo_snapshot();
        if (changed)
          apply_affine();
      }

      {
        const bool changed = ImGui::SliderScalar("Base Y", ImGuiDataType_Double, &m_underlay_base.y, &min_v, &max_v, "%.4f",
                                                 ImGuiSliderFlags_ClampOnInput);
        if (ImGui::IsItemActivated())
          m_view->push_undo_snapshot();
        if (changed)
          apply_affine();
      }

      auto vector_component = [&](const char* label, double* p_data)
      {
        const bool changed = ImGui::InputDouble(label, p_data, 0.0, 0.0, "%.4f");
        if (ImGui::IsItemActivated())
          m_view->push_undo_snapshot();

        if (changed)
          apply_affine();
      };

      ImGui::TextUnformatted("+U vector");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Maps the horizontal (width / columns) of the source bitmap. From the Base, this vector reaches the "
                          "right edge of the full image. Set X from edge and datum tools primarily affect +U.");
      vector_component("U X", &m_underlay_u.x);
      vector_component("U Y", &m_underlay_u.y);

      ImGui::TextUnformatted("+V vector");
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "Maps the vertical (height / rows) of the source bitmap. From the Base, this vector reaches the bottom edge of the "
            "full image. 'Set Y from edge...' and y-axis calibration adjust +V (this is what you set for the Y-axis).");

      vector_component("V X", &m_underlay_v.x);
      vector_component("V Y", &m_underlay_v.y);

      // Live derived metrics (useful while tweaking shear cases).
      {
        const double lu        = std::hypot(m_underlay_u.x, m_underlay_u.y);
        const double lv        = std::hypot(m_underlay_v.x, m_underlay_v.y);
        double       shear_deg = 0.0;
        if (lu > 1e-12 && lv > 1e-12)
        {
          const double dot = m_underlay_u.x * m_underlay_v.x + m_underlay_u.y * m_underlay_v.y;
          const double c   = std::clamp(dot / (lu * lv), -1.0, 1.0);
          shear_deg        = std::acos(c) * (180.0 / std::acos(-1.0));
        }

        ImGui::Text("U len: %.4f   V len: %.4f   Angle U-V: %.2f deg", lu, lv, shear_deg);
      }

      if (ImGui::Button("Make orthogonal (keep lengths + U direction)"))
      {
        m_view->push_undo_snapshot();
        force_underlay_orthogonal_(sk);
      }

      if (ui_show_contextual_help() && ImGui::IsItemHovered())
        ImGui::SetTooltip("Project V to be perpendicular to U. Keeps the current lengths of both axes and the original "
                          "orientation (sign of U cross V). After this the Center / Half / Rotation sliders return.");
    }
  }
}

void GUI::on_sketch_underlay_file(const std::string& file_path, const std::string& file_bytes)
{
  Sketch::sptr sk = m_underlay_import_sketch_target.lock();
  m_underlay_import_sketch_target.reset();
  if (!sk)
    sk = m_view->curr_sketch_shared();

  m_view->push_undo_snapshot();
  uint8_t hr, hg, hb, ha;
  underlay_highlight_color_rgba(hr, hg, hb, ha);
  if (!sk->underlay().load_from_file_bytes(file_bytes, m_view->asset_store(), hr, hg, hb, ha, sk->get_plane(),
                                           sk->is_visible()))
  {
    show_message("Could not decode image: " + std::filesystem::path(file_path).filename().string());
    return;
  }

  m_underlay_calib_x_done = false;
  m_underlay_calib_y_done = false;
  m_underlay_panel_sketch = nullptr;
  show_message("Underlay: " + std::filesystem::path(file_path).filename().string());
}

void GUI::cancel_underlay_calib_()
{
  m_dist_callback        = nullptr;
  m_underlay_calib_phase = Underlay_calib_phase::None;

  m_underlay_calib_sketch_wk.reset();
}

void GUI::force_underlay_orthogonal_(const Sketch::sptr& sk)
{
  if (!sk || !sk->underlay().has_image())
    return;

  Sketch_underlay& ul = sk->underlay();
  gp_Pnt2d         b;
  gp_Vec2d         au, av;
  ul.get_affine(b, au, av);
  const double lu = au.Magnitude();
  const double lv = av.Magnitude();
  if (lu > 1e-12 && lv > 1e-12)
  {
    gp_Vec2d au_n = au;
    au_n.Normalize();
    const double det = au.X() * av.Y() - au.Y() * av.X();
    gp_Vec2d     av_perp;
    if (det >= 0.0)
      av_perp = gp_Vec2d(-au_n.Y() * lv, au_n.X() * lv);
    else
      av_perp = gp_Vec2d(au_n.Y() * lv, -au_n.X() * lv);

    const gp_Vec2d au_final = au_n * lu;
    ul.set_affine_plane(b, au_final, av_perp, sk->get_plane(), sk->is_visible());
    // Refresh locals for this frame and any immediate follow-up UI.
    m_underlay_u = dvec2(au_final.X(), au_final.Y());
    m_underlay_v = dvec2(av_perp.X(), av_perp.Y());
  }
  ul.ui_params(m_underlay_center.x, m_underlay_center.y, m_underlay_half_extents.x, m_underlay_half_extents.y, m_underlay_rot);
  m_underlay_panel_sketch = nullptr;
}

void GUI::begin_underlay_calib_set_x_(const Sketch::sptr& sk)
{
  if (!sk || !sk->underlay().has_image())
    return;

  if (m_view->curr_sketch_shared() != sk)
  {
    show_message("Make this sketch current in the sketch list, then try again.");
    return;
  }

  cancel_underlay_calib_();
  sk->underlay().ui_params(m_underlay_center.x, m_underlay_center.y, m_underlay_half_extents.x, m_underlay_half_extents.y,
                           m_underlay_rot);
  m_underlay_calib_sketch_wk = sk;
  m_underlay_calib_phase     = Underlay_calib_phase::PickX1;
  show_message("Underlay X: uses the current transform. Click bitmap corner (0,0), then along +U; then enter the distance.");
}

void GUI::begin_underlay_calib_set_y_(const Sketch::sptr& sk)
{
  if (!sk || !sk->underlay().has_image())
    return;

  if (m_view->curr_sketch_shared() != sk)
  {
    show_message("Make this sketch current in the sketch list, then try again.");
    return;
  }

  cancel_underlay_calib_();
  sk->underlay().ui_params(m_underlay_center.x, m_underlay_center.y, m_underlay_half_extents.x, m_underlay_half_extents.y,
                           m_underlay_rot);

  m_underlay_calib_sketch_wk = sk;
  m_underlay_calib_phase     = Underlay_calib_phase::PickY1;
  show_message("Underlay Y: uses the current transform. Click two points along +V; then enter the drawing distance.");
}

void GUI::underlay_calib_prompt_x_distance_(const Sketch::sptr& sk)
{
  m_underlay_calib_phase       = Underlay_calib_phase::AwaitDistX;
  const double       L_model   = m_underlay_calib_x0.Distance(m_underlay_calib_x1);
  const float        dist_show = static_cast<float>(L_model / m_view->get_dimension_scale());
  const ScreenCoords spos      = cursor_screen_coords();

  Sketch::wptr wk      = sk;
  auto         on_dist = [this, wk](float new_dist, bool is_final)
  {
    if (!is_final)
      return;

    const Sketch::sptr s = wk.lock();
    if (!s || !s->underlay().has_image())
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
    if (!s->underlay().rescale_uv_chord_to_length(m_underlay_calib_x0, m_underlay_calib_x1, Dx, s->get_plane(),
                                                  s->is_visible()))
    {
      m_view->pop_undo_snapshot();
      show_message("Could not calibrate X (underlay axes degenerate or segment too short).");
      return;
    }

    s->underlay().ui_params(m_underlay_center.x, m_underlay_center.y, m_underlay_half_extents.x, m_underlay_half_extents.y,
                            m_underlay_rot);

    m_underlay_calib_x_done = true;
    m_underlay_panel_sketch = nullptr;

    if (m_underlay_calib_x_done && m_underlay_calib_y_done)
      force_underlay_orthogonal_(s);

    m_dist_callback        = nullptr;
    m_underlay_calib_phase = Underlay_calib_phase::None;
    show_message("X distance applied to the picked segment. Use Set Y from edge for the vertical span if needed, or adjust "
                 "transforms.");
  };

  set_dist_edit(dist_show, std::move(on_dist), spos);
}

void GUI::underlay_calib_prompt_y_distance_(const Sketch::sptr& sk)
{
  m_underlay_calib_phase       = Underlay_calib_phase::AwaitDistY;
  const double       L_model   = m_underlay_calib_y0.Distance(m_underlay_calib_y1);
  const float        dist_show = static_cast<float>(L_model / m_view->get_dimension_scale());
  const ScreenCoords spos      = cursor_screen_coords();

  Sketch::wptr wk      = sk;
  auto         on_dist = [this, wk](float new_dist, bool is_final)
  {
    if (!is_final)
      return;

    const Sketch::sptr s = wk.lock();
    if (!s || !s->underlay().has_image())
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
    if (!s->underlay().rescale_v_chord_to_length(m_underlay_calib_y0, m_underlay_calib_y1, Dy, s->get_plane(), s->is_visible()))
    {
      m_view->pop_undo_snapshot();
      show_message("Set Y: picks need a clear span along image height (not along the same edge as X only). Try two points "
                   "further apart in V.");
      return;
    }

    s->underlay().ui_params(m_underlay_center.x, m_underlay_center.y, m_underlay_half_extents.x, m_underlay_half_extents.y,
                            m_underlay_rot);

    m_underlay_calib_y_done = true;
    m_underlay_panel_sketch = nullptr;

    if (m_underlay_calib_x_done && m_underlay_calib_y_done)
      force_underlay_orthogonal_(s);

    m_dist_callback = nullptr;
    cancel_underlay_calib_();
    show_message("Y distance applied to the picked segment. Use Set X from edge for the horizontal span if needed, or adjust "
                 "transforms.");
  };

  set_dist_edit(dist_show, std::move(on_dist), spos);
}

bool GUI::try_underlay_calib_click_(const ScreenCoords& screen_coords)
{
  if (m_underlay_calib_phase == Underlay_calib_phase::None)
    return false;

  if (m_underlay_calib_phase == Underlay_calib_phase::AwaitDistX || m_underlay_calib_phase == Underlay_calib_phase::AwaitDistY)
    return true;

  const Sketch::sptr sk     = m_underlay_calib_sketch_wk.lock();
  const Sketch::sptr sk_cur = m_view->curr_sketch_shared();
  if (!sk || !sk_cur || sk != sk_cur)
  {
    cancel_underlay_calib_();
    show_message("Underlay calibration canceled.");
    return true;
  }

  if (!sk->underlay().has_image())
  {
    cancel_underlay_calib_();
    return true;
  }

  const std::optional<gp_Pnt2d> pt =
      Sketch_underlay::pick_point_for_calib(*m_view, sk->get_plane(), sk->get_nodes(), screen_coords);
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
    m_underlay_calib_x0    = *pt;
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
    m_underlay_calib_y0    = *pt;
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
  if (!show_shape_list_effective())
  {
    m_view->set_shape_list_hover(nullptr);
    return;
  }

  float max_name_text_w = 0.f;
  for (const Shp_ptr& s : m_view->get_shapes())
  {
    EZY_ASSERT(s);
    const std::string& nm = s->get_name();
    max_name_text_w       = std::max(max_name_text_w, ImGui::CalcTextSize(nm.c_str(), nm.c_str() + nm.size()).x);
  }

  if (!ImGui::Begin("Shape List", &m_show_shape_list, ImGuiWindowFlags_None))
  {
    m_view->set_shape_list_hover(nullptr);
    ImGui::End();
    return;
  }

  ImGui::BeginChild("##shape_list_scroll", ImVec2(0.f, 0.f), false, ImGuiWindowFlags_HorizontalScrollbar);

  const float name_field_w = list_name_field_width_(ImGui::GetStyle(), max_name_text_w);
  int         index        = 0;

  // Add checkbox for hiding all shapes except current sketches
  if (ImGui::Checkbox("Hide all", &m_hide_all_shapes))
  {
    if (m_hide_all_shapes)
      m_view->set_shape_list_hover(nullptr);
    // Update visibility of all shapes based on the new state
    for (const Shp_ptr& shape : m_view->get_shapes())
      shape->set_visible(!m_hide_all_shapes);
  }

  ImGui::Separator();

  const std::vector<std::string>& mat_names       = occt_material_combo_labels_();
  const int                       nmat            = static_cast<int>(mat_names.size());
  float                           mat_label_w_max = 0.0f;
  for (int mi = 0; mi < nmat; ++mi)
    mat_label_w_max = std::max(mat_label_w_max, ImGui::CalcTextSize(mat_names[static_cast<size_t>(mi)].c_str()).x);

  const ImGuiStyle& st_mat      = ImGui::GetStyle();
  const float       mat_popup_w = std::min(440.0f, std::max(280.0f, mat_label_w_max + st_mat.WindowPadding.x * 2.0f +
                                                                        st_mat.FramePadding.x * 2.0f + st_mat.ScrollbarSize + 8.0f));

  Shp_ptr shape_to_delete;
  Shp_ptr shape_list_hover;

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
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(header.x, header.y, header.z, 0.40f));
      ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(header.x, header.y, header.z, 0.55f));
      ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(header.x, header.y, header.z, 0.65f));
      const ImVec4 text = ImGui::GetStyleColorVec4(ImGuiCol_Text);

      // Shape List: when a row matches OCCT selection, ImGuiCol_Text is nudged brighter (RGB only).
      constexpr float k_shape_list_selected_text_rgb_scale = 1.08f; // per-channel multiplier for a modest relative lift
      constexpr float k_shape_list_selected_text_rgb_bias =
          0.04f; // added after scaling so very dark text still reads a bit lighter

      ImGui::PushStyleColor(
          ImGuiCol_Text,
          ImVec4(std::min(1.0f, text.x * k_shape_list_selected_text_rgb_scale + k_shape_list_selected_text_rgb_bias),
                 std::min(1.0f, text.y * k_shape_list_selected_text_rgb_scale + k_shape_list_selected_text_rgb_bias),
                 std::min(1.0f, text.z * k_shape_list_selected_text_rgb_scale + k_shape_list_selected_text_rgb_bias), text.w));
    }

    ImGui::BeginGroup();

    // Unique ID suffix using index
    std::string id_suffix = "##" + std::to_string(index++);
    // Editable text box for name
    char name_buffer[1024];
    safe_cstr_copy(name_buffer, sizeof(name_buffer), shape->get_name().c_str());

    int mat_idx = shape->Material();
    if (mat_idx < 0 || mat_idx >= nmat)
      mat_idx = static_cast<int>(m_view->get_default_material().Name());

    auto apply_shape_material = [&](int i)
    {
      if (i < 0 || i >= nmat)
        return;

      shape->SetMaterial(Graphic3d_MaterialAspect(static_cast<Graphic3d_NameOfMaterial>(i)));
      m_view->refresh_shape_shading_(shape);
      m_view->ctx().Redisplay(shape, true);
      m_view->ctx().UpdateCurrentViewer();
    };

    ImGui::PushID(("name" + id_suffix).c_str());
    ImGui::SetNextItemWidth(name_field_w);
    if (ImGui::InputText("", name_buffer, sizeof(name_buffer)))
      shape->set_name(std::string(name_buffer));

    if (ImGui::BeginPopupContextItem("shape_name_ctx"))
    {
      if (ImGui::MenuItem("Shape info..."))
        open_shape_info_(shape);

      if (ImGui::MenuItem("Delete"))
        shape_to_delete = shape;

      ImGui::EndPopup();
    }

    ImGui::PopID();
    // Visibility checkbox
    ImGui::SameLine();
    ImGui::PushID(("visible" + id_suffix).c_str());
    bool visible = shape->get_visible();
    if (ImGui::Checkbox("", &visible))
    {
      if (!visible && m_view->shape_list_hover() == shape)
        m_view->set_shape_list_hover(nullptr);

      shape->set_visible(visible);
    }

    if (ui_show_contextual_help() && ImGui::IsItemHovered())
      ImGui::SetTooltip("visibility");

    ImGui::PopID();

    // Shaded display mode checkbox
    ImGui::SameLine();
    ImGui::PushID(("shaded" + id_suffix).c_str());
    bool shaded = shape->get_disp_mode() == AIS_Shaded;
    if (ImGui::Checkbox("", &shaded))
      shape->set_disp_mode(shaded ? AIS_Shaded : AIS_WireFrame);

    if (ui_show_contextual_help() && ImGui::IsItemHovered())
      ImGui::SetTooltip("solid/wire");

    ImGui::PopID();

    ImGui::SameLine();
    ImGui::PushID(("matbtn" + id_suffix).c_str());
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, ImGui::GetStyle().FramePadding.y));
    if (ImGui::Button("M"))
      ImGui::OpenPopup("mat_pick");

    ImGui::PopStyleVar();
    if (ui_show_contextual_help() && ImGui::IsItemHovered())
      ImGui::SetTooltip("%s\n(click: material; right-click name: Delete)", mat_names[static_cast<size_t>(mat_idx)].c_str());

    ImGui::SetNextWindowSize(ImVec2(mat_popup_w, 0.0f), ImGuiCond_Appearing);
    if (ImGui::BeginPopup("mat_pick"))
    {
      ImGui::TextUnformatted("Material");
      ImGui::Separator();
      const float max_h = ImGui::GetTextLineHeightWithSpacing() * 12.0f;
      const float sc_w  = std::max(1.0f, ImGui::GetContentRegionAvail().x);
      if (ImGui::BeginChild("mat_sc", ImVec2(sc_w, max_h), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysVerticalScrollbar))
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
      if (ImGui::MenuItem("Shape info..."))
        open_shape_info_(shape);

      if (ImGui::MenuItem("Delete"))
        shape_to_delete = shape;

      ImGui::EndPopup();
    }

    ImGui::PopID();

    ImGui::EndGroup();

    if (ImGui::IsItemHovered() && shape->get_visible())
      shape_list_hover = shape;

    if (row_selected)
    {
      ImGui::PopStyleColor(4);
      if (ui_show_contextual_help() && ImGui::IsItemHovered())
        ImGui::SetTooltip("Selected in 3D viewer");
    }
  }

  m_view->set_shape_list_hover(shape_list_hover);

  if (shape_to_delete)
    m_view->delete_shapes({shape_to_delete});

  ImGui::EndChild();
  ImGui::End();
}

void GUI::open_shape_info_(const Shp_ptr& shape)
{
  if (shape.IsNull())
    return;

  const std::vector<std::string>& mat_names = occt_material_combo_labels_();
  int                             mat_idx   = shape->Material();
  if (mat_idx < 0 || mat_idx >= static_cast<int>(mat_names.size()))
    mat_idx = static_cast<int>(m_view->get_default_material().Name());

  shp_info::Display_meta meta;
  meta.name         = shape->get_name();
  meta.material     = mat_names[static_cast<size_t>(mat_idx)];
  meta.display_mode = shape->get_disp_mode() == AIS_Shaded ? "Shaded" : "Wireframe";
  meta.visible      = shape->get_visible();

  m_shape_info_shp   = shape;
  m_shape_info_lines = shp_info::collect(shape->Shape(), &meta);
  m_shape_info_open  = true;
}

void GUI::shape_info_dialog_()
{
  if (!m_shape_info_open)
    return;

  if (m_shape_info_shp.IsNull())
  {
    m_shape_info_open = false;
    return;
  }

  bool shape_still_exists = false;
  for (const Shp_ptr& s : m_view->get_shapes())
    if (s == m_shape_info_shp)
    {
      shape_still_exists = true;
      break;
    }

  if (!shape_still_exists)
  {
    m_shape_info_open = false;
    m_shape_info_shp.Nullify();
    m_shape_info_lines.clear();
    return;
  }

  const std::string title = "Shape info: " + m_shape_info_shp->get_name();
  ImGui::SetNextWindowSize(ImVec2(440.0f, 0.0f), ImGuiCond_FirstUseEver);
  bool open = m_shape_info_open;
  if (!ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_None))
  {
    m_shape_info_open = open;
    ImGui::End();
    return;
  }

  if (ImGui::Button("Refresh"))
    open_shape_info_(m_shape_info_shp);

  ImGui::Separator();

  const float max_h = ImGui::GetTextLineHeightWithSpacing() * 18.0f;
  if (ImGui::BeginChild("shape_info_scroll", ImVec2(0.0f, max_h), ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_AlwaysVerticalScrollbar))
  {
    if (ImGui::BeginTable("shape_info_tbl", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg))
    {
      ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
      ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

      for (const shp_info::Line& line : m_shape_info_lines)
      {
        if (line.label.empty() && line.value.empty())
        {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::Separator();
          ImGui::TableSetColumnIndex(1);
          ImGui::Separator();
          continue;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(line.label.c_str());
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(line.value.c_str());
      }

      ImGui::EndTable();
    }

    ImGui::EndChild();
  }

  m_shape_info_open = open;
  if (!open)
    m_shape_info_shp.Nullify();

  ImGui::End();
}

float GUI::list_name_field_width_(const ImGuiStyle& st, const float max_name_text_w)
{
  constexpr float k_name_field_cap = 480.f;
  // Keep names editable even when all names are short.
  constexpr float k_name_field_floor = 72.f;
  const float     name_pad_x         = st.FramePadding.x * 2.0f;
  return std::clamp(max_name_text_w + name_pad_x, k_name_field_floor, k_name_field_cap);
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
  ImGui::Text("Undo: %zu (Ctrl+Z)  |  Redo: %zu (Ctrl+Y)  [max 50]", m_view->undo_stack_size(), m_view->redo_stack_size());
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
  if (elapsed > 6000) // 6 seconds
  {
    m_message_visible = false;
    m_message.clear();
    return;
  }

  // Calculate fade effect (optional, for visual polish)
  float alpha = 1.0f;
  if (elapsed > 2500)                            // Start fading after 2.5 seconds
    alpha = 1.0f - (elapsed - 2500.0f) / 500.0f; // Fade over last 0.5 seconds

  // Set window position (bottom-right corner)
  ImGuiIO& io = ImGui::GetIO();
  ImVec2   window_pos(io.DisplaySize.x - 250.0f, io.DisplaySize.y - 50.0f); // Adjust as needed
  ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(240.0f, 40.0f), ImGuiCond_Once);

  // Begin window with minimal flags
  ImGui::Begin("MessageStatus##unique_id", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);

  // Set text color with alpha for fade effect
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, alpha));
  ImGui::TextWrapped("%s", m_message.c_str());
  ImGui::PopStyleColor();

  ImGui::End();
}

// Log window implementation
namespace
{

std::string format_log_line(const std::string& base, size_t repeat_count)
{
  if (repeat_count <= 1)
    return base;
  return base + " #" + std::to_string(repeat_count);
}

void append_log_line(std::vector<char>& buffer, const std::string& line)
{
  if (buffer.size() > 1)
  {
    buffer.pop_back(); // trailing '\0'
    buffer.push_back('\n');
  }
  else if (!buffer.empty())
    buffer.pop_back();

  buffer.insert(buffer.end(), line.begin(), line.end());
  buffer.push_back('\0');
}

} // namespace

void GUI::log_message(const std::string& message)
{
  if (!m_log_last_line_base.empty() && message == m_log_last_line_base)
  {
    ++m_log_repeat_count;
    const std::string line = format_log_line(m_log_last_line_base, m_log_repeat_count);
    m_log_buffer.resize(m_log_last_line_start);
    m_log_buffer.insert(m_log_buffer.end(), line.begin(), line.end());
    m_log_buffer.push_back('\0');
    m_log_scroll_to_bottom = true;
    return;
  }

  append_log_line(m_log_buffer, message);
  m_log_last_line_base   = message;
  m_log_last_line_start  = m_log_buffer.size() - message.size() - 1;
  m_log_repeat_count     = 1;
  m_log_scroll_to_bottom = true;
}

void GUI::log_window_()
{
  if (!log_window_visible_effective())
    return;

  if (!ImGui::Begin("Log", &m_log_window_visible, ImGuiWindowFlags_None))
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
  if (!show_lua_console_effective())
    return;
  if (!m_lua_console)
    m_lua_console = std::make_unique<Lua_console>(this);
  m_lua_console->render(&m_show_lua_console);
}

void GUI::python_console_()
{
  if (!show_python_console_effective())
    return;

  if (!m_python_console)
    m_python_console = std::make_unique<Python_console>(this);
  m_python_console->render(&m_show_python_console);
}

void GUI::ensure_python_console()
{
#ifdef EZYCAD_HAVE_PYTHON
  if (!m_python_console)
    m_python_console = std::make_unique<Python_console>(this);
#endif
}

Python_console* GUI::get_python_console() { return m_python_console.get(); }

void GUI::init(GLFWwindow* window, ImFont* console_font)
{
  m_glfw_window  = window;
  m_console_font = console_font;
  initialize_toolbar_();
  settings::set_log_callback([this](const std::string& m) { log_message(m); });
  setup_log_redirection_(); // Set up stream redirection
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
    try
    {
      const std::filesystem::path p(m_last_opened_project_path);
      if (std::filesystem::exists(p))
      {
        std::ifstream file(p, std::ios::binary);
        if (file.is_open())
        {
          const std::string file_bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
          if (is_valid_project_file_(file_bytes))
          {
            log_message("EzyCad: loading last opened project: " + p.string());
            on_file(p.string(), file_bytes, false);
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
#endif

  const std::string user_startup = settings::load_user_startup_project();
  if (is_valid_project_file_(user_startup))
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

  const std::string file_bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
  if (is_valid_project_file_(file_bytes))
  {
    log_message("EzyCad: loading bundled default project (" + std::string(k_bundled_default) + ").");
    on_file(k_bundled_default, file_bytes, false);
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

std::vector<uint8_t> GUI::serialized_project_ezy_() const
{
  const std::string manifest = serialized_project_json_();
  return pack_ezy(manifest, m_view->asset_store());
}

void GUI::save_startup_project_()
{
  const std::vector<uint8_t> ezy_bytes = serialized_project_ezy_();
  if (!settings::save_user_startup_project(ezy_bytes))
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

    // clang-format off
    case Mode::Sketch_add_node:
    case Mode::Sketch_add_edge:
    case Mode::Sketch_add_multi_edges:
    case Mode::Sketch_operation_axis:
    case Mode::Sketch_add_square:
    case Mode::Sketch_add_rectangle:
    case Mode::Sketch_add_rectangle_center_pt:
    case Mode::Sketch_add_circle:
    case Mode::Sketch_add_slot:
    case Mode::Sketch_add_seg_circle_arc:
    case Mode::Sketch_dim_anno:           m_view->curr_sketch().sketch_pt_move(screen_coords);  break;
    case Mode::Sketch_face_extrude:       m_view->sketch_face_extrude(screen_coords, true);     break;
    // clang-format on
  default:
    break;
  }
}

void GUI::mirror_selected_sketch_edges() { m_view->curr_sketch().mirror_selected_edges(); }

void GUI::on_left_click_(const ScreenCoords& screen_coords)
{
  switch (m_mode)
  {
    // clang-format off
  case Mode::Move:                m_view->shp_move().finalize();                      break;
  case Mode::Rotate:              m_view->shp_rotate().finalize();                    break;
  case Mode::Scale:               m_view->shp_scale().finalize();                     break;
  case Mode::Sketch_face_extrude: m_view->sketch_face_extrude(screen_coords, false);  break;
    // clang-format on

  case Mode::Sketch_add_node:
  case Mode::Sketch_add_edge:
  case Mode::Sketch_add_multi_edges:
  case Mode::Sketch_add_seg_circle_arc:
  case Mode::Sketch_add_square:
  case Mode::Sketch_add_rectangle:
  case Mode::Sketch_add_rectangle_center_pt:
  case Mode::Sketch_add_circle:
  case Mode::Sketch_add_slot:
    hide_dist_edit();
    m_view->curr_sketch().add_sketch_pt(screen_coords);
    break;

  case Mode::Sketch_operation_axis:
    // Only treat LMB as "place axis point" while *defining* the axis.
    // Once an axis exists, we stay in this mode so the Options panel can show
    // the Mirror/Revolve/Clear buttons. Further clicks must be allowed to
    // select sketch edges/faces (via AIS selection) without redefining the axis.
    // To redefine, the user must first use the "Clear axis" button.
    if (!m_view->curr_sketch().has_operation_axis())
    {
      hide_dist_edit();
      m_view->curr_sketch().add_sketch_pt(screen_coords);
    }
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
}

void GUI::sketch_left_click(const ScreenCoords& screen_coords)
{
  m_view->on_mouse_move(screen_coords);
  on_left_click_(screen_coords);
}

void GUI::on_mouse_button(int button, int action, int mods)
{
  const ScreenCoords screen_coords = cursor_screen_coords();

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
    on_left_click_(screen_coords);

  else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && mods == 0)
    // Right button is to finalize the current operation.
    switch (m_mode)
    {
    case Mode::Sketch_add_edge:
      Sketch::set_edge_from_center(m_edge_from_center);
      Sketch::set_add_mid_pt_edges(m_add_mid_pt_line_edges);
      m_view->curr_sketch().finalize_elm();
      break;

    case Mode::Sketch_add_multi_edges:
      Sketch::set_add_mid_pt_edges(m_add_mid_pt_line_edges);
      m_view->curr_sketch().finalize_elm();
      break;

    default:
      break;
    }
}

void GUI::on_mouse_scroll(double xoffset, double yoffset)
{
  EZY_ASSERT(m_glfw_window != nullptr);

  const bool shift_finer = glfwGetKey(m_glfw_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                           glfwGetKey(m_glfw_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

  m_view->on_mouse_scroll(xoffset, yoffset, shift_finer);
}

void GUI::on_resize(int width, int height) { m_view->on_resize(width, height); }

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
  std::string download_name{"export.step"};
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
  char const* selected           = tinyfd_openFileDialog("Import STEP or PLY", "", 3, filter_patterns, "STEP / PLY files", 0);

  if (selected)
  {
    // Binary mode required for PLY (mesh payload may contain 0x1A; Windows text mode treats that as EOF).
    std::ifstream file(selected, std::ios::binary);
    if (!file.is_open())
    {
      show_message("Error opening: " + std::filesystem::path(selected).filename().string());
      return;
    }

    const std::string file_bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    if (!file_bytes.empty())
      on_import_file(selected, file_bytes);
    else
      show_message("Error opening: " + std::filesystem::path(selected).filename().string());
  }
#else
  import_file_dialog_async();
#endif
}

// New, open, save related
void GUI::new_project_()
{
  m_last_saved_path.clear();
  m_view->new_file();
}

void GUI::open_file_dialog_()
{
#ifndef __EMSCRIPTEN__
  // Native: Use tinyfiledialogs
  char const* filter_patterns[1] = {"*.ezy"};                                   // Restrict to .ezy files
  char const* selected           = tinyfd_openFileDialog("Open EzyCad project", // Dialog title
                                                         "",                    // Default path (empty for OS default)
                                                         1,                     // Number of filter patterns
                                                         filter_patterns,       // Filter patterns (*.ezy)
                                                         "EzyCad Files",        // Filter description
                                                         0                      // Single file selection
            );
  if (selected)
  {
    std::ifstream     file(selected, std::ios::binary);
    const std::string file_bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    if (file.good() && !file_bytes.empty())
      on_file(selected, file_bytes);

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
  const std::vector<uint8_t> ezy_bytes = serialized_project_ezy_();

#ifndef __EMSCRIPTEN__
  std::string file;
  if (!m_last_saved_path.empty())
    file = m_last_saved_path; // Reuse last saved path
  else
  {
    char const* filter_patterns[1] = {"*.ezy"};
    char const* selected = tinyfd_saveFileDialog("Save EzyCad Project", "project.ezy", 1, filter_patterns, "EzyCad Files");
    if (selected)
    {
      file              = selected;
      m_last_saved_path = file; // Update last saved path
    }
  }
  if (!file.empty())
  {
    std::ofstream out(file, std::ios::binary);
    if (out.is_open())
    {
      out.write(reinterpret_cast<const char*>(ezy_bytes.data()), static_cast<std::streamsize>(ezy_bytes.size()));
      out.close();
      show_message("Saved: " + std::filesystem::path(file).filename().string());
    }
    else
      show_message("Failed to save: " + std::filesystem::path(file).filename().string());
  }
  else
    show_message("Save canceled");
#else
  std::string default_file =
      m_last_saved_path.empty() ? "project.ezy" : std::filesystem::path(m_last_saved_path).filename().string();
  save_file_dialog_async("Save EzyCad project", default_file, ezy_bytes);
#endif
}

void GUI::on_file(const std::string& file_path, const std::string& file_bytes, bool announce_load)
{
  using namespace nlohmann;

  log_message("on_file: path=" + file_path + " bytes=" + std::to_string(file_bytes.size()) +
              " zip=" + (is_ezy_zip(file_bytes) ? "yes" : "no") + " json=" + (is_ezy_json(file_bytes) ? "yes" : "no"));

  m_view->push_undo_snapshot();
  const std::optional<std::string> manifest = manifest_from_project_file_(file_bytes, *m_view, true);
  if (!manifest)
    log_message("on_file: manifest_from_project_file_ FAILED");
  else
    log_message("on_file: manifest ok, len=" + std::to_string(manifest->size()) +
                " valid=" + (is_valid_project_manifest_(*manifest) ? "yes" : "no"));

  if (!manifest || !is_valid_project_manifest_(*manifest))
  {
    m_view->pop_undo_snapshot();
    show_message("Invalid EzyCad project: " + std::filesystem::path(file_path).filename().string());
    return;
  }

  const json j = json::parse(*manifest);
  m_view->load(*manifest);
  log_message("on_file: load complete");
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
          var contents = new Uint8Array(e.target.result);
          var fileName = file.name;
          // Allocate heap memory for contents
          var length      = contents.length;
          var contentsPtr = _malloc(length);
          HEAPU8.set(contents, contentsPtr);
          // Call C++ callback with path and contents
          Module.ccall('on_file_selected', null, [ 'string', 'number', 'number' ], [ fileName, contentsPtr, length ]);
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
          Module.ccall('on_import_file_selected', null, [ 'string', 'number', 'number' ], [ fileName, contentsPtr, length ]);
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
          Module.ccall('on_sketch_underlay_selected', null, [ 'string', 'number', 'number' ],
                       [ fileName, contentsPtr, length ]);
          _free(contentsPtr);
        };
        reader.readAsArrayBuffer(file);
      }
      document.body.removeChild(input);
    };
    input.click();
  });
}
void GUI::save_file_dialog_async(const char* title, const std::string& default_file, const std::vector<uint8_t>& ezy_bytes)
{
  (void)title;
  EM_ASM_ARGS(
      {
        var data = HEAPU8.subarray($0, $0 + $1);
        var blob = new Blob([data],
                            {
                              type:
                                'application/octet-stream'
                            });
        var url    = URL.createObjectURL(blob);
        var a      = document.createElement('a');
        a.href     = url;
        a.download = UTF8ToString($2); // Default file name
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
        Module.ccall('on_save_file_selected', null, ['string'], [UTF8ToString($2)]);
      },
      ezy_bytes.data(), ezy_bytes.size(), default_file.c_str());
}
void GUI::note_saved_project_filename(const std::string& filename)
{
  if (!filename.empty())
    m_last_saved_path = filename;
}
void GUI::download_blob_async(const std::string& default_filename, const std::string& data)
{
  EM_ASM_ARGS(
      {
        var blob = new Blob([HEAPU8.subarray($0, $0 + $1)],
                            {
                              type:
                                'application/octet-stream'
                            });
        var url    = URL.createObjectURL(blob);
        var a      = document.createElement('a');
        a.href     = url;
        a.download = UTF8ToString($2);
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
      },
      data.data(), data.size(), default_filename.c_str());
}
// C-style callback for Emscripten
extern "C" void on_file_selected(const char* file_path, char* contents, int length)
{
  const std::string file_bytes(contents, static_cast<size_t>(length));
  GUI::instance().on_file(file_path, file_bytes);
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
  GUI& g = GUI::instance();
  if (file_name)
    g.note_saved_project_filename(file_name);

  g.show_message(std::string("Saved: ") + (file_name ? file_name : ""));
}
#endif
