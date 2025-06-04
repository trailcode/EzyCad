#ifdef _WIN32
// Suppress warning C4005: 'APIENTRY': macro redefinition
// This occurs because windows.h defines APIENTRY which may conflict with other definitions
#include <windows.h>
#endif

#include "gui.h"

#include <array>
#include <map>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else

#include "third_party/tinyfiledialogs/tinyfiledialogs.h"
#endif

#include "imgui.h"
#include "occt_view.h"
#include "sketch.h"
#include "utl.h"

GUI* gui_instance = nullptr;

GUI::GUI()
{
  m_view = std::make_unique<Occt_view>(*this);
  DO_ASSERT(!gui_instance);
  gui_instance = this;
  
}

GUI::~GUI()
{
  cleanup_log_redirection_();  // Clean up stream redirection
}

#ifdef __EMSCRIPTEN__
GUI& GUI::instance()
{
  DO_ASSERT(gui_instance);
  return *gui_instance;
}
#endif

void GUI::render_gui()
{
  menu_bar_();
  toolbar_();
  dist_edit_();
  sketch_list_();
  shape_list_();
  options_();
  message_status_window_();
  log_window_();
#ifndef NDEBUG
  dbg_();
#endif
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
      {                        Mode::Normal,      Mode::Normal},
      {                          Mode::Move,      Mode::Normal},
      {                         Mode::Scale,      Mode::Normal},
      {                        Mode::Rotate,      Mode::Normal},
      {                   Mode::Sketch_inspection_mode,      Mode::Normal},
      {              Mode::Sketch_from_face,      Mode::Normal},
      {           Mode::Sketch_face_extrude,      Mode::Normal},
      {                 Mode::Shape_chamfer,      Mode::Normal},
      {                  Mode::Shape_fillet,      Mode::Normal},
      {         Mode::Shape_polar_duplicate,      Mode::Normal},
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
      DO_ASSERT(parent_modes.find(Mode(idx)) != parent_modes.end());

    return true;
  }();

  const auto itr = parent_modes.find(get_mode());
  DO_ASSERT(itr != parent_modes.end());
  set_mode(itr->second);
}

void GUI::on_key(int key, int scancode, int action, int mods)
{
  const ScreenCoords screen_coords(glm::dvec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y));

  if (action == 1)
  {
    switch (key)
    {
      case GLFW_KEY_D:
        m_view->delete_selected();
        break;
      case GLFW_KEY_G:
        set_mode(Mode::Move);
        break;
      case GLFW_KEY_E:
        set_mode(Mode::Sketch_face_extrude);
        break;
      case GLFW_KEY_L:
        break;
      case GLFW_KEY_TAB:
        m_view->dimension_input(screen_coords);
        break;
      case GLFW_KEY_ESCAPE:
        m_view->cancel(Set_parent_mode::Yes);
        hide_dist_edit();
        break;
      case GLFW_KEY_ENTER:
        m_view->on_enter(screen_coords);
        hide_dist_edit();
        break;
    }
  }
}

// Initialize toolbar buttons
void GUI::initialize_toolbar_()
{
  m_toolbar_buttons = {
      {                           load_texture("User.png"),  true,                  "Inspection mode",                         Mode::Normal},
      {             load_texture("Assembly_AxialMove.png"), false,                   "Shape move (g)",                           Mode::Move},
      {                   load_texture("Draft_Rotate.png"), false,                     "Shape rotate",                         Mode::Rotate},
      {                     load_texture("Part_Scale.png"), false,                      "Shape Scale",                          Mode::Scale},
      {        load_texture("Workbench_Sketcher_none.png"), false,           "Sketch inspection mode",                    Mode::Sketch_inspection_mode},
      {          load_texture("Macro_FaceToSketch_48.png"), false,        "Create a sketch from face",               Mode::Sketch_from_face},
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
      {              load_texture("PartDesign_Fillet.png"), false,                           "Fillet",                  Mode::Shape_chamfer},
      {               load_texture("Draft_PolarArray.png"), false,            "Shape polar duplicate",          Mode::Shape_polar_duplicate},
      {                       load_texture("Part_Cut.png"), false,                        "Shape cut",                   Command::Shape_cut},
      {                      load_texture("Part_Fuse.png"), false,                       "Shape fuse",                  Command::Shape_fuse},
      {                    load_texture("Part_Common.png"), false,                     "Shape common",                Command::Shape_common},
  };
}

void GUI::menu_bar_()
{
  if (!ImGui::BeginMainMenuBar())
    return;

  if (ImGui::BeginMenu("File"))
  {
    if (ImGui::MenuItem("New"))
    {
    }
    if (ImGui::MenuItem("Open", "Ctrl+O"))
      open_file_dialog_();

    if (ImGui::MenuItem("Save", "Ctrl+S"))
      save_file_dialog_();

    if (ImGui::MenuItem("Save as"))
    {
      m_last_saved_path.clear();  // Force dialog
      save_file_dialog_();
    }

    if (ImGui::MenuItem("Import"))
      import_file_dialog_();

    if (ImGui::MenuItem("Exit"))
      exit(0);

    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("View"))
  {
    if (ImGui::MenuItem("Geos Tests"))
    {
      // s_showGeosTestPanel ^= 1;
    }

    ImGui::EndMenu();
  }

  ImGui::EndMainMenuBar();
}

// Render toolbar with ImGui
void GUI::toolbar_()
{
  // ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
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
            DO_ASSERT(false);
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
  if (ImGui::InputFloat("##float_value", &m_dist_val, 0.0f, 0.0f, "%.2f"))
    m_dist_callback(m_dist_val);

  ImGui::End();
}

void GUI::sketch_list_()
{
  if (m_show_sketch_list)
  {
    ImGui::Begin("Sketch List", &m_show_sketch_list, ImGuiWindowFlags_None);

    int index = 0;
    std::shared_ptr<Sketch> sketch_to_delete;
    for (std::shared_ptr<Sketch>& sketch : m_view->get_sketches())
    {
      DO_ASSERT(sketch);

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
}

void GUI::shape_list_()
{
  int         index = 0;
  static bool show  = true;
  if (show)
  {
    ImGui::Begin("Shape List", &show, ImGuiWindowFlags_None);

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
}

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

void GUI::options_()
{
  ImGui::Begin("Options", nullptr, ImGuiWindowFlags_NoCollapse);

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
    case Mode::Sketch_operation_axis: options_sketch_operation_axis_mode_();  break;
    case Mode::Shape_chamfer:         options_shape_chamfer_mode_();          break;
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

void GUI::options_normal_mode_()
{
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
  ImGui::TextUnformatted("Move axis:");

  Move_options& opts = m_view->shp_move().get_opts();

  ImGui::Checkbox("X", &opts.constrain_axis_x);
  ImGui::SameLine();
  ImGui::Checkbox("Y", &opts.constrain_axis_y);
  ImGui::SameLine();
  ImGui::Checkbox("Z", &opts.constrain_axis_z);
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

  float chamfer_dist = float(m_view->shp_chamfer().get_chamfer_dist());
  if (ImGui::InputFloat("Chamfer dist##float_value", &chamfer_dist, 0.0f, 0.0f, "%.2f"))
    m_view->shp_chamfer().set_chamfer_dist(chamfer_dist);
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
    polar_dup.set_num_selms(num_elms);

  if (ImGui::Checkbox("Rotate dups", &rotate_dups))
    polar_dup.set_rotate_dups(rotate_dups);

  if (ImGui::Checkbox("Combine dups", &combine_dups))
    polar_dup.set_combine_dups(combine_dups);

  if (ImGui::Button("Dup"))
    if (Status s = polar_dup.dup(); !s.is_ok())
      show_message(s.message());
}

void GUI::dbg_()
{
  ImGui::Begin("dbg", nullptr, ImGuiWindowFlags_NoCollapse);
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
  m_log_messages.push_back(message);
  m_log_scroll_to_bottom = true;  // Auto-scroll to new message
}

void GUI::log_window_()
{
  ImGui::Begin("Log", &m_log_window_visible, ImGuiWindowFlags_NoCollapse);

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

void GUI::set_dist_edit(float dist, const ScreenCoords& screen_coords, std::function<void(float)>&& callback)
{
  m_dist_val      = dist;
  m_dist_edit_loc = screen_coords;
  m_dist_callback = std::move(callback);
}

void GUI::init(GLFWwindow* window)
{
  initialize_toolbar_();
  setup_log_redirection_();  // Set up stream redirection
  m_view->init_window(window);
  m_view->init_viewer();
  m_view->init_default();
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
      m_view->sketch_face_extrude(screen_coords);
      break;
    default:
      break;
  }
}

void GUI::on_mouse_button(int button, int action, int mods)
{
  const ScreenCoords screen_coords(glm::dvec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y));

  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && mods == 0)
    switch (m_mode)
    {
      case Mode::Move:
        m_view->shp_move().finalize_move_selected();
        break;

      case Mode::Sketch_add_edge:
      case Mode::Sketch_add_multi_edges:
      case Mode::Sketch_add_seg_circle_arc:
      case Mode::Sketch_add_square:
      case Mode::Sketch_operation_axis:
      case Mode::Sketch_add_circle:
      case Mode::Sketch_add_slot:
        hide_dist_edit();
        m_view->curr_sketch().add_sketch_pt(screen_coords);
        break;
      case Mode::Shape_chamfer:
        if (Status s = m_view->shp_chamfer().add_chamfer(screen_coords, m_chamfer_mode); !s.is_ok())
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

  m_view->on_mouse_button(button, action, mods);
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
  m_cout_log_buf = new LogStreamBuf(*this, m_original_cout_buf);
  m_cerr_log_buf = new LogStreamBuf(*this, m_original_cerr_buf);

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
  const std::string json_str = m_view->to_json();
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
  // TODO update GUI title to include file name.
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
