// Tool mode switching, keyboard routing, and Options panel (mode-specific UI).

#include <GLFW/glfw3.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "geom.h"
#include "gui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "modes.h"
#include "occt_view.h"
#include "sketch.h"
#include "utl_occt.h"

#include <gp_Pnt2d.hxx>

using namespace glm;

namespace
{

constexpr ImGuiTableFlags k_options_table_flags          = ImGuiTableFlags_SizingFixedFit;
constexpr float           k_options_control_col_w        = 148.f;
constexpr float           k_options_sketch_control_col_w = 176.f;

void options_table_setup_columns_(float label_col_w, float control_col_w)
{
  ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, label_col_w);
  ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthFixed, control_col_w);
}

void options_right_aligned_label_(const char* text)
{
  ImGui::AlignTextToFramePadding();
  const float text_w  = ImGui::CalcTextSize(text).x;
  const float col_w   = ImGui::GetColumnWidth();
  const float x0      = ImGui::GetCursorPosX();
  const float right_x = x0 + std::max(0.0f, col_w - text_w - ImGui::GetStyle().CellPadding.x * 2.0f);
  ImGui::SetCursorPosX(right_x);
  ImGui::TextUnformatted(text);
}

// Up to `max_frac` digits after the decimal, strip trailing zeros (and a trailing '.').
void format_double_trim_fraction(char* dst, std::size_t dst_sz, double v, int max_frac)
{
  char tmp[64];
  std::snprintf(tmp, sizeof tmp, "%.*f", max_frac, v);
  std::size_t len = std::strlen(tmp);
  while (len > 0 && tmp[len - 1] == '0')
    --len;
  if (len > 0 && tmp[len - 1] == '.')
    --len;
  tmp[len] = '\0';
  if (std::strcmp(tmp, "-0") == 0)
    std::snprintf(dst, dst_sz, "0");
  else
    std::snprintf(dst, dst_sz, "%s", tmp);
}

} // namespace

std::string GUI::get_doc_url_for_mode(Mode mode)
{
  static const std::unordered_map<Mode, std::string> doc_urls = {
      // clang-format off
      {Mode::Normal,                          "https://ezycad.readthedocs.io/en/latest/usage.html#user-interface"},
      {Mode::Move,                            "https://ezycad.readthedocs.io/en/latest/usage.html#shape-move-tool-g"},
      {Mode::Rotate,                          "https://ezycad.readthedocs.io/en/latest/usage.html#shape-rotate-tool-r"},
      {Mode::Scale,                           "https://ezycad.readthedocs.io/en/latest/usage.html#shape-scale-tool-s"},
      {Mode::Sketch_inspection_mode,          "https://ezycad.readthedocs.io/en/latest/usage-sketch.html#2d-sketching"}, // general entry point for sketch tools; no narrower section currently
      {Mode::Sketch_from_planar_face,         "https://ezycad.readthedocs.io/en/latest/usage-sketch.html#create-sketch-from-planar-face-tool"},
      {Mode::Sketch_face_extrude,             "https://ezycad.readthedocs.io/en/latest/usage.html#extrude-sketch-face-tool-e"},
      {Mode::Shape_chamfer,                   "https://ezycad.readthedocs.io/en/latest/usage.html#other-feature-operations"},
      {Mode::Shape_fillet,                    "https://ezycad.readthedocs.io/en/latest/usage.html#other-feature-operations"},
      {Mode::Shape_polar_duplicate,           "https://ezycad.readthedocs.io/en/latest/usage.html#shape-polar-duplicate-tool"},
      {Mode::Sketch_add_node,                 "https://ezycad.readthedocs.io/en/latest/usage-sketch.html#add-node-tool"},
      {Mode::Sketch_add_edge,                 "https://ezycad.readthedocs.io/en/latest/usage-sketch.html#single-line-edge-tool"},
      {Mode::Sketch_add_multi_edges,          "https://ezycad.readthedocs.io/en/latest/usage-sketch.html#multi-line-edge-tool"},
      {Mode::Sketch_add_seg_circle_arc,       "https://ezycad.readthedocs.io/en/latest/usage-sketch.html#arc-segment-creation-tool"},
      {Mode::Sketch_operation_axis,           "https://ezycad.readthedocs.io/en/latest/usage-sketch.html#operation-axis-tool"},
      {Mode::Sketch_add_square,               "https://ezycad.readthedocs.io/en/latest/usage-sketch.html#square-tool"},
      {Mode::Sketch_add_rectangle,            "https://ezycad.readthedocs.io/en/latest/usage-sketch.html#rectangle-tool-two-points"},
      {Mode::Sketch_add_rectangle_center_pt,  "https://ezycad.readthedocs.io/en/latest/usage-sketch.html#rectangle-tool-center-point"},
      {Mode::Sketch_add_circle,               "https://ezycad.readthedocs.io/en/latest/usage-sketch.html#circle-creation-tools"},
      {Mode::Sketch_add_circle_3_pts,         ""}, // planned feature - no specific section in the docs yet; falls back to main guide
      {Mode::Sketch_add_slot,                 "https://ezycad.readthedocs.io/en/latest/usage-sketch.html#slot-creation-tool"},
      {Mode::Sketch_dim_anno,                 "https://ezycad.readthedocs.io/en/latest/usage-sketch.html#dimension-tool"},
      // clang-format on
  };

  EZY_ASSERT_MSG(doc_urls.size() == static_cast<std::size_t>(Mode::_count),
                 "get_doc_url_for_mode: doc_urls map size does not match Mode::_count");

  auto it = doc_urls.find(mode);
  if (it != doc_urls.end() && !it->second.empty())
  {
    return it->second;
  }
  // fallback to main guide
  return "https://ezycad.readthedocs.io/en/latest/usage.html";
}

void GUI::options_doc_help_button_()
{
  ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
  if (ImGui::SmallButton("?##options_pane_help"))
    open_url_(GUI::get_doc_url_for_mode(get_mode()));
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("Open the relevant section of the online user guide.");
}

void GUI::set_mode(Mode mode)
{
  cancel_underlay_calib_();
  m_mode = mode;
  m_view->on_mode();
  for (Toolbar_button& b : m_toolbar_buttons)
    if (b.data.index() == 0)
      b.is_active = std::get<Mode>(b.data) == mode;
}

void GUI::set_parent_mode()
{
  static std::map<Mode, Mode> parent_modes = {
      {Mode::Normal, Mode::Normal},
      {Mode::Move, Mode::Normal},
      {Mode::Scale, Mode::Normal},
      {Mode::Rotate, Mode::Normal},
      {Mode::Sketch_inspection_mode, Mode::Normal},
      {Mode::Sketch_from_planar_face, Mode::Normal},
      {Mode::Sketch_face_extrude, Mode::Normal},
      {Mode::Shape_chamfer, Mode::Normal},
      {Mode::Shape_fillet, Mode::Normal},
      {Mode::Shape_polar_duplicate, Mode::Normal},
      {Mode::Sketch_add_node, Mode::Sketch_inspection_mode},
      {Mode::Sketch_add_edge, Mode::Sketch_inspection_mode},
      {Mode::Sketch_add_multi_edges, Mode::Sketch_inspection_mode},
      {Mode::Sketch_add_seg_circle_arc, Mode::Sketch_inspection_mode},
      {Mode::Sketch_operation_axis, Mode::Sketch_inspection_mode},
      {Mode::Sketch_add_square, Mode::Sketch_inspection_mode},
      {Mode::Sketch_add_rectangle, Mode::Sketch_inspection_mode},
      {Mode::Sketch_add_rectangle_center_pt, Mode::Sketch_inspection_mode},
      {Mode::Sketch_add_circle, Mode::Sketch_inspection_mode},
      {Mode::Sketch_add_circle_3_pts, Mode::Sketch_inspection_mode},
      {Mode::Sketch_add_slot, Mode::Sketch_inspection_mode},
      {Mode::Sketch_dim_anno, Mode::Sketch_inspection_mode},
  };

  static bool check = [&]()
  {
    // Called only once.
    for (size_t idx = 0; idx < size_t(Mode::_count); ++idx)
      EZY_ASSERT(parent_modes.find(Mode(idx)) != parent_modes.end());

    return true;
  }();

  (void)check;
  const auto itr = parent_modes.find(get_mode());
  EZY_ASSERT(itr != parent_modes.end());
  set_mode(itr->second);
}

void GUI::on_key(int key, int scancode, int action, int mods)
{
  (void)scancode;
  const bool press_or_repeat = (action == GLFW_PRESS || action == GLFW_REPEAT);

  // Zoom (+/-): scaled like mouse wheel; GLFW_REPEAT while held; Shift = Blender-style finer step.
  if (press_or_repeat && (mods & (GLFW_MOD_CONTROL | GLFW_MOD_ALT)) == 0)
  {
    bool zoom_in  = false;
    bool zoom_out = false;
    // Shift+= is the main-keyboard zoom-in path; Shift is structural (produces '+'), not "finer step" intent.
    bool zoom_in_shift_is_structural = false;
    switch (key)
    {
    case GLFW_KEY_KP_ADD:
      zoom_in = true;
      break;
    case GLFW_KEY_KP_SUBTRACT:
    case GLFW_KEY_MINUS:
      zoom_out = true;
      break;
    case GLFW_KEY_EQUAL:
      if ((mods & GLFW_MOD_SHIFT) != 0)
      {
        zoom_in                     = true;
        zoom_in_shift_is_structural = true;
      }
      break;
    default:
      break;
    }

    const bool shift_finer = ((mods & GLFW_MOD_SHIFT) != 0) && !zoom_in_shift_is_structural;

    if (zoom_in)
    {
      m_view->zoom_view_wheel_notches(1.0, shift_finer);
      return;
    }
    else if (zoom_out)
    {
      m_view->zoom_view_wheel_notches(-1.0, shift_finer);
      return;
    }
  }

  // Blender-style view roll: Shift + NumPad 4/6, main 4/6, or Left/Right (NumLock-on numpad often maps here).
  // Use PRESS and REPEAT (like zoom) so hold-to-repeat works; route before keypad digit -> selection filter.
  if (press_or_repeat && (mods & GLFW_MOD_SHIFT) != 0 && (mods & (GLFW_MOD_CONTROL | GLFW_MOD_ALT)) == 0)
  {
    const bool roll_ccw = (key == GLFW_KEY_KP_4 || key == GLFW_KEY_4 || key == GLFW_KEY_LEFT);
    const bool roll_cw  = (key == GLFW_KEY_KP_6 || key == GLFW_KEY_6 || key == GLFW_KEY_RIGHT);
    if (roll_ccw || roll_cw)
    {
      const double step = m_view_roll_step_deg;
      m_view->roll_view_z_deg(roll_ccw ? -step : step);
      return;
    }
  }

  if (action != GLFW_PRESS)
    return;

  // Nearest world-axis orthographic view (roll zero). Routes before Normal-mode keypad selection filters.
  if (key == GLFW_KEY_KP_5 && (mods & (GLFW_MOD_SHIFT | GLFW_MOD_CONTROL | GLFW_MOD_ALT)) == 0)
  {
    m_view->snap_view_to_nearest_standard_axis();
    return;
  }

  // Orbit like trihedron / LMB orbit (AIS_ViewController axes); step matches Settings view rotation step.
  if ((mods & (GLFW_MOD_SHIFT | GLFW_MOD_CONTROL | GLFW_MOD_ALT)) == 0)
  {
    const double step = m_view_roll_step_deg;
    // clang-format off
    switch (key)
    {
      case GLFW_KEY_KP_8: m_view->orbit_view_screen_step_deg(0.0, step);  return;
      case GLFW_KEY_KP_2: m_view->orbit_view_screen_step_deg(0.0, -step); return;
      case GLFW_KEY_KP_4: m_view->orbit_view_screen_step_deg(step, 0.0);  return;
      case GLFW_KEY_KP_6: m_view->orbit_view_screen_step_deg(-step, 0.0); return;
      default: break;
    }
    // clang-format on
  }

  const ScreenCoords screen_coords(dvec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y));

  bool ctrl_pressed = (mods & GLFW_MOD_CONTROL) != 0;
  if (ctrl_pressed)
  {
    switch (key)
    {
    case GLFW_KEY_N:
      new_project_();
      break;

    case GLFW_KEY_O:
      open_file_dialog_();
      break;

    case GLFW_KEY_S:
      save_file_dialog_();
      break;

    case GLFW_KEY_Z:
      if ((mods & GLFW_MOD_SHIFT) != 0)
        m_view->redo();
      else
        m_view->undo();
      break;

    case GLFW_KEY_Y:
      m_view->redo();
      break;

    default:
      break;
    }
  }
  else
  {
    // -------------------------------------------------------------------------
    // Shape selection filter hotkeys (Options -> Selection Mode combo, Normal mode only)
    //
    // Maps main-row 1-9 and keypad 1-9 to TopAbs_ShapeEnum values 0..8 in OCCT order
    // (same order as c_names_TopAbs_ShapeEnum in utl_occt.h):
    //   1 Compound, 2 CompSolid, 3 Solid, 4 Shell, 5 Face, 6 Wire, 7 Edge, 8 Vertex, 9 Shape
    //
    // Input routing: main.cpp calls GUI::on_key only when !io.WantTextInput, so digits go to
    // text fields while typing. We return early so these keys are not handled again below.
    //
    // Other modes: chamfer/fillet/sketch may override selection mode via Occt_view::on_mode().
    // -------------------------------------------------------------------------
    if (get_mode() == Mode::Normal)
    {
      int idx = -1;
      if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9)
        idx = key - GLFW_KEY_1;

      else if (key >= GLFW_KEY_KP_1 && key <= GLFW_KEY_KP_9)
        idx = key - GLFW_KEY_KP_1;

      if (idx >= 0 && idx <= static_cast<int>(TopAbs_SHAPE))
      {
        m_view->set_shp_selection_mode(static_cast<TopAbs_ShapeEnum>(idx));
        return;
      }
    }

    switch (key)
    {
    case GLFW_KEY_ESCAPE:
      cancel_underlay_calib_();
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

      // clang-format off
    case GLFW_KEY_D:
      if ((mods & GLFW_MOD_SHIFT) != 0)
        m_view->delete_selected();
      else
        set_mode(Mode::Sketch_dim_anno);
      break;

    case GLFW_KEY_DELETE:
    case GLFW_KEY_BACKSPACE:
      m_view->delete_selected();
      break;

    case GLFW_KEY_G: set_mode(Mode::Move); break;
    case GLFW_KEY_R: set_mode(Mode::Rotate); break;
    case GLFW_KEY_E: set_mode(Mode::Sketch_face_extrude); break;
    case GLFW_KEY_S: set_mode(Mode::Scale); break;
    case GLFW_KEY_C: set_mode(Mode::Shape_chamfer); break;
    case GLFW_KEY_F: set_mode(Mode::Shape_fillet); break;
      // clang-format on
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

void GUI::options_()
{
  if (!show_options_effective())
    return;

  if (!ImGui::Begin("Options", &m_show_options))
  {
    ImGui::End();
    return;
  }

  ImGui::BeginChild("##options_scroll", ImVec2(0.f, 0.f), false, ImGuiWindowFlags_HorizontalScrollbar);

  // clang-format off
  switch (get_mode())
  {
    case Mode::Normal:                          options_normal_mode_();                       break;
    case Mode::Move:                            options_move_mode_();                         break;
    case Mode::Rotate:                          options_rotate_mode_();                       break;
    case Mode::Scale:                           options_scale_mode_();                        break;
    case Mode::Shape_chamfer:                   options_shape_chamfer_mode_();                break;
    case Mode::Shape_fillet:                    options_shape_fillet_mode_();                 break;
    case Mode::Shape_polar_duplicate:           options_shape_polar_duplicate_mode_();        break;
    
      // Sketch related modes:
    case Mode::Sketch_inspection_mode:          options_sketch_inspection_mode_();            break;
    case Mode::Sketch_from_planar_face:         options_sketch_from_planer_face_mode_();      break;
    case Mode::Sketch_operation_axis:           options_sketch_operation_axis_mode_();        break;
    case Mode::Sketch_face_extrude:             options_sketch_face_extrude_mode_();          break;
    case Mode::Sketch_dim_anno:                 options_sketch_dim_anno_mode_();              break;
    case Mode::Sketch_add_node:                 options_sketch_add_node_mode_();              break;
    case Mode::Sketch_add_edge:                 options_sketch_add_edge_mode_();              break;
    case Mode::Sketch_add_multi_edges:          options_sketch_add_multi_line_edge_mode_();   break;
    case Mode::Sketch_add_seg_circle_arc:       options_sketch_add_arc_circle_mode_();        break;
    case Mode::Sketch_add_square:               options_sketch_add_square_mode_();            break;
    case Mode::Sketch_add_rectangle:            options_sketch_add_rectangle_mode_();         break;
    case Mode::Sketch_add_rectangle_center_pt:  options_sketch_add_rectangle_center_mode_();  break;
    case Mode::Sketch_add_circle:               options_sketch_add_circle_mode_();            break;
    case Mode::Sketch_add_circle_3_pts:         options_sketch_add_circle_three_pts_mode_();  break;
    case Mode::Sketch_add_slot:                 options_sketch_add_slot_mode_();              break;
    default:
      break;
  }
  // clang-format on

  ImGui::EndChild();
  ImGui::End();
}

void GUI::options_sketch_inspection_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Sketch_inspection_mode);

  options_sketch_common_();
}

void GUI::options_normal_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Normal);

  ImGui::TextUnformatted(current_mode_description());
  options_doc_help_button_();
  ImGui::Separator();

  float label_col_w = ImGui::CalcTextSize("Selection Mode").x;
  label_col_w += ImGui::GetStyle().CellPadding.x * 2.0f + 8.0f;

  ImGui::TextUnformatted("Selection");
  if (ImGui::BeginTable("options_normal_selection", 2, k_options_table_flags))
  {
    options_table_setup_columns_(label_col_w, k_options_control_col_w);

    int current_item = static_cast<int>(m_view->get_shp_selection_mode());
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    options_right_aligned_label_("Selection Mode");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::BeginCombo("##selection_mode", c_names_TopAbs_ShapeEnum[current_item].data(), ImGuiComboFlags_HeightSmall))
    {
      for (int i = 0; i < static_cast<int>(c_names_TopAbs_ShapeEnum.size()); i++)
        if (ImGui::Selectable(c_names_TopAbs_ShapeEnum[i].data(), current_item == i))
          m_view->set_shp_selection_mode(static_cast<TopAbs_ShapeEnum>(i));

      ImGui::EndCombo();
    }
    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
    if (ui_show_help(2))
    {
      ImGui::TextDisabled("(?)");
      if (ImGui::IsItemHovered())
      {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextDisabled("Hotkeys: 1-9 (Normal mode) set filter when the 3D view has focus, not while typing in UI.");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
      }
    }

    ImGui::EndTable();
  }

  ImGui::Separator();
  ImGui::TextUnformatted("Material");
  const std::vector<std::string>& material_names = occt_material_combo_labels_();
  int                             current_mat    = int(m_view->get_default_material().Name());
  if (current_mat < 0 || current_mat >= static_cast<int>(material_names.size()))
    current_mat = 0;
  const float material_row_w = label_col_w + k_options_control_col_w;
  ImGui::SetNextItemWidth(std::max(ImGui::GetContentRegionAvail().x, material_row_w));
  if (ImGui::BeginCombo("##default_material_normal", material_names[static_cast<size_t>(current_mat)].data(),
                        ImGuiComboFlags_HeightSmall))
  {
    for (int i = 0; i < static_cast<int>(material_names.size()); i++)
      if (ImGui::Selectable(material_names[static_cast<size_t>(i)].data(), current_mat == i))
      {
        Graphic3d_MaterialAspect mat(static_cast<Graphic3d_NameOfMaterial>(i));
        m_view->set_default_material(mat);
      }
    ImGui::EndCombo();
  }

  options_orthographic_projection_();
}

void GUI::options_move_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Move);

  ImGui::TextUnformatted(current_mode_description());
  options_doc_help_button_();
  ImGui::Separator();
  ImGui::TextUnformatted("Constrain axis:");

  Move_options& opts = m_view->shp_move().get_opts();

  ImGui::Checkbox("X", &opts.constr_axis_x);
  ImGui::SameLine();
  ImGui::Checkbox("Y", &opts.constr_axis_y);
  ImGui::SameLine();
  ImGui::Checkbox("Z", &opts.constr_axis_z);

  options_orthographic_projection_();
}

void GUI::options_scale_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Scale);

  ImGui::TextUnformatted(current_mode_description());
  options_doc_help_button_();
  ImGui::Separator();

  options_orthographic_projection_();
}

void GUI::options_rotate_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Rotate);

  ImGui::TextUnformatted(current_mode_description());
  options_doc_help_button_();
  ImGui::Separator();

  int selected_axis = static_cast<int>(m_view->shp_rotate().get_rotation_axis());
  if (ImGui::RadioButton("View to object axis", &selected_axis, static_cast<int>(Rotation_axis::View_to_object)))
    m_view->shp_rotate().set_rotation_axis(Rotation_axis::View_to_object);

  if (ImGui::RadioButton("Around X axis", &selected_axis, static_cast<int>(Rotation_axis::X_axis)))
    m_view->shp_rotate().set_rotation_axis(Rotation_axis::X_axis);

  if (ImGui::RadioButton("Around Y axis", &selected_axis, static_cast<int>(Rotation_axis::Y_axis)))
    m_view->shp_rotate().set_rotation_axis(Rotation_axis::Y_axis);

  if (ImGui::RadioButton("Around Z axis", &selected_axis, static_cast<int>(Rotation_axis::Z_axis)))
    m_view->shp_rotate().set_rotation_axis(Rotation_axis::Z_axis);

  options_orthographic_projection_();
}

void GUI::options_shape_chamfer_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Shape_chamfer);

  float label_col_w = std::max(ImGui::CalcTextSize("Chamfer Mode").x, ImGui::CalcTextSize("Chamfer dist").x);
  label_col_w += ImGui::GetStyle().CellPadding.x * 2.0f + 8.0f;

  ImGui::TextUnformatted(current_mode_description());
  options_doc_help_button_();
  ImGui::Separator();

  if (ImGui::BeginTable("options_chamfer_tool", 2, k_options_table_flags))
  {
    options_table_setup_columns_(label_col_w, k_options_control_col_w);

    int current_mode = static_cast<int>(m_chamfer_mode);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    options_right_aligned_label_("Chamfer Mode");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::Combo("##chamfer_mode", &current_mode, c_chamfer_mode_strs.data(), (int)c_chamfer_mode_strs.size()))
    {
      m_chamfer_mode = static_cast<Chamfer_mode>(current_mode);
      m_view->on_chamfer_mode();
    }

    static char  chamfer_buf[64];
    const double scale        = m_view->get_dimension_scale();
    const double chamfer_dist = m_view->shp_chamfer().get_chamfer_dist() / scale;

    ImGui::PushID("chamfer_dist_micron");
    const ImGuiID chamfer_input_id = ImGui::GetID("##micron");
    ImGuiContext* ctx              = ImGui::GetCurrentContext();
    if (ctx && ctx->ActiveId != chamfer_input_id)
      format_double_trim_fraction(chamfer_buf, sizeof chamfer_buf, chamfer_dist, 6);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    options_right_aligned_label_("Chamfer dist");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(100.0f);
    constexpr ImGuiInputTextFlags k_dim_flags = ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsScientific;
    if (ImGui::InputText("##micron", chamfer_buf, sizeof chamfer_buf, k_dim_flags))
    {
      char*        end = nullptr;
      const double p   = std::strtod(chamfer_buf, &end);
      if (end != chamfer_buf)
      {
        while (*end == ' ' || *end == '\t')
          ++end;
        if (*end == '\0')
          m_view->shp_chamfer().set_chamfer_dist(p * scale);
      }
    }
    ImGui::PopID();
    ImGui::EndTable();
  }

  options_orthographic_projection_();
}

void GUI::options_shape_fillet_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Shape_fillet);

  float label_col_w = std::max(ImGui::CalcTextSize("Fillet Mode").x, ImGui::CalcTextSize("Fillet radius").x);
  label_col_w += ImGui::GetStyle().CellPadding.x * 2.0f + 8.0f;

  ImGui::TextUnformatted(current_mode_description());
  options_doc_help_button_();
  ImGui::Separator();

  if (ImGui::BeginTable("options_fillet_tool", 2, k_options_table_flags))
  {
    options_table_setup_columns_(label_col_w, k_options_control_col_w);

    int current_mode = static_cast<int>(m_fillet_mode);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    options_right_aligned_label_("Fillet Mode");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::Combo("##fillet_mode", &current_mode, c_fillet_mode_strs.data(), (int)c_fillet_mode_strs.size()))
    {
      m_fillet_mode = static_cast<Fillet_mode>(current_mode);
      m_view->on_fillet_mode();
    }

    static char  fillet_buf[64];
    const double scale         = m_view->get_dimension_scale();
    const double fillet_radius = m_view->shp_fillet().get_fillet_radius() / scale;

    ImGui::PushID("fillet_rad_micron");
    const ImGuiID fillet_input_id = ImGui::GetID("##micron");
    ImGuiContext* ctx             = ImGui::GetCurrentContext();
    if (ctx && ctx->ActiveId != fillet_input_id)
      format_double_trim_fraction(fillet_buf, sizeof fillet_buf, fillet_radius, 6);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    options_right_aligned_label_("Fillet radius");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(100.0f);
    constexpr ImGuiInputTextFlags k_dim_flags = ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsScientific;
    if (ImGui::InputText("##micron", fillet_buf, sizeof fillet_buf, k_dim_flags))
    {
      char*        end = nullptr;
      const double p   = std::strtod(fillet_buf, &end);
      if (end != fillet_buf)
      {
        while (*end == ' ' || *end == '\t')
          ++end;

        if (*end == '\0')
          m_view->shp_fillet().set_fillet_radius(p * scale);
      }
    }
    ImGui::PopID();

    ImGui::EndTable();
  }

  options_orthographic_projection_();
}

void GUI::options_shape_polar_duplicate_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Shape_polar_duplicate);

  auto& polar_dup    = m_view->shp_polar_dup();
  float polar_angle  = float(polar_dup.get_polar_angle());
  int   num_elms     = int(polar_dup.get_num_elms());
  bool  rotate_dups  = polar_dup.get_rotate_dups();
  bool  combine_dups = polar_dup.get_combine_dups();

  float label_col_w = ImGui::CalcTextSize("Polar angle").x;
  label_col_w       = std::max(label_col_w, ImGui::CalcTextSize("Num Elms").x);
  label_col_w       = std::max(label_col_w, ImGui::CalcTextSize("Rotate dups").x);
  label_col_w       = std::max(label_col_w, ImGui::CalcTextSize("Combine dups").x);
  label_col_w       = std::max(label_col_w, ImGui::CalcTextSize("Material").x);
  label_col_w += ImGui::GetStyle().CellPadding.x * 2.0f + 8.0f;

  ImGui::TextUnformatted(current_mode_description());
  options_doc_help_button_();
  ImGui::Separator();

  if (ImGui::BeginTable("options_polar_dup_tool", 2, k_options_table_flags))
  {
    options_table_setup_columns_(label_col_w, k_options_control_col_w);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    options_right_aligned_label_("Polar angle");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::InputFloat("##polar_angle", &polar_angle, 0.0f, 0.0f, "%.2f"))
      polar_dup.set_polar_angle(polar_angle);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    options_right_aligned_label_("Num Elms");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::InputInt("##num_elms", &num_elms))
      polar_dup.set_num_elms(num_elms);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    options_right_aligned_label_("Rotate dups");
    ImGui::TableSetColumnIndex(1);
    if (ImGui::Checkbox("##rotate_dups", &rotate_dups))
      polar_dup.set_rotate_dups(rotate_dups);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    options_right_aligned_label_("Combine dups");
    ImGui::TableSetColumnIndex(1);
    if (ImGui::Checkbox("##combine_dups", &combine_dups))
      polar_dup.set_combine_dups(combine_dups);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(1);
    if (ImGui::Button("Dup"))
      if (Status s = polar_dup.dup(); !s.is_ok())
        show_message(s.message());

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    options_right_aligned_label_("Material");
    ImGui::TableSetColumnIndex(1);
    const std::vector<std::string>& material_names = occt_material_combo_labels_();
    int                             current_item   = int(m_view->get_default_material().Name());
    if (current_item < 0 || current_item >= static_cast<int>(material_names.size()))
      current_item = 0;

    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::BeginCombo("##default_material_polar_dup", material_names[static_cast<size_t>(current_item)].data(),
                          ImGuiComboFlags_HeightSmall))
    {
      for (int i = 0; i < static_cast<int>(material_names.size()); i++)
        if (ImGui::Selectable(material_names[static_cast<size_t>(i)].data(), current_item == i))
        {
          Graphic3d_MaterialAspect mat(static_cast<Graphic3d_NameOfMaterial>(i));
          m_view->set_default_material(mat);
        }
      ImGui::EndCombo();
    }

    ImGui::EndTable();
  }

  options_orthographic_projection_();
}

void GUI::options_sketch_from_planer_face_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Sketch_from_planar_face);

  ImGui::TextUnformatted(current_mode_description());
  options_doc_help_button_();
  ImGui::Separator();

  options_orthographic_projection_();
}

void GUI::options_sketch_operation_axis_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Sketch_operation_axis);

  options_sketch_common_();

  if (m_view->curr_sketch().has_operation_axis())
  {
    if (ImGui::Button("Mirror"))
      m_view->curr_sketch().mirror_selected_edges();

    static float revolve_angle = 360.0f;

    if (ImGui::Button("Revolve"))
      m_view->revolve_selected(to_radians(revolve_angle));

    ImGui::SetNextItemWidth(80.0f);
    ImGui::SameLine();
    ImGui::InputFloat("##float_value", &revolve_angle, 0.0f, 0.0f, "%.2f");

    if (ImGui::Button("Clear axis"))
      m_view->curr_sketch().clear_operation_axis();
  }

  options_sketch_len_angle_hotkeys_();
}

void GUI::options_sketch_face_extrude_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Sketch_face_extrude);

  options_sketch_common_();

  if (ImGui::BeginTable("options_sketch_extrude", 2, k_options_table_flags))
  {
    options_table_setup_columns_(options_sketch_label_col_w_(), k_options_sketch_control_col_w);

    bool extrude_both_sides = m_view->shp_extrude().get_both_sides();
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    options_right_aligned_label_("Both sides");
    ImGui::TableSetColumnIndex(1);
    if (ImGui::Checkbox("##extrude_both_sides", &extrude_both_sides))
      m_view->shp_extrude().set_both_sides(extrude_both_sides);

    const std::vector<std::string>& material_names = occt_material_combo_labels_();
    int                             current_item   = int(m_view->get_default_material().Name());
    if (current_item < 0 || current_item >= static_cast<int>(material_names.size()))
      current_item = 0;

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    options_right_aligned_label_("Material");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::BeginCombo("##default_material_sketch_extrude", material_names[static_cast<size_t>(current_item)].data(),
                          ImGuiComboFlags_HeightSmall))
    {
      for (int i = 0; i < static_cast<int>(material_names.size()); i++)
        if (ImGui::Selectable(material_names[static_cast<size_t>(i)].data(), current_item == i))
        {
          Graphic3d_MaterialAspect mat(static_cast<Graphic3d_NameOfMaterial>(i));
          m_view->set_default_material(mat);
        }
      ImGui::EndCombo();
    }

    ImGui::EndTable();
  }
}

void GUI::options_sketch_dim_anno_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Sketch_dim_anno);

  options_sketch_common_();
}

void GUI::options_sketch_add_node_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Sketch_add_node);

  options_sketch_common_();

  if (ui_show_help(3))
  {
    ImGui::Separator();
    ImGui::TextUnformatted("Shortcuts");

    ImGui::TextWrapped("TAB: type length along the rubber band. Shift+TAB: type angle (degrees, CCW from +X).");
    ImGui::TextWrapped("Snap the first click to an existing sketch point to start a rubber band, then click to place the node "
                       "(or press Enter after typing a length). An unsnapped click still places a single node immediately.");
  }
}

void GUI::options_sketch_add_edge_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Sketch_add_edge);

  Sketch::set_add_mid_pt_edges(m_add_mid_pt_edges);

  options_sketch_common_();
  options_sketch_len_angle_hotkeys_();

  ImGui::Separator();
  ImGui::TextUnformatted("Options");
  {
    bool add_mids = m_add_mid_pt_edges;
    if (ImGui::Checkbox("Add midpoint nodes", &add_mids))
    {
      m_add_mid_pt_edges = add_mids;
      Sketch::set_add_mid_pt_edges(add_mids);
    }

    if (ui_show_help(2) && ImGui::IsItemHovered())
      ImGui::SetTooltip("When on, new linear edges get an automatic midpoint node (used for snapping to edge centers). Default "
                        "is off (no midpoints added).");
  }
}

void GUI::options_sketch_add_multi_line_edge_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Sketch_add_multi_edges);

  Sketch::set_add_mid_pt_edges(m_add_mid_pt_edges);

  options_sketch_common_();
  options_sketch_len_angle_hotkeys_();

  Sketch::set_add_mid_pt_edges(m_add_mid_pt_edges);

  ImGui::Separator();
  ImGui::TextUnformatted("Options");
  {
    bool add_mids = m_add_mid_pt_edges;
    if (ImGui::Checkbox("Add midpoint nodes", &add_mids))
    {
      m_add_mid_pt_edges = add_mids;
      Sketch::set_add_mid_pt_edges(add_mids);
    }

    if (ui_show_help(2) && ImGui::IsItemHovered())
      ImGui::SetTooltip("When on, new linear edges get an automatic midpoint node (used for snapping to edge centers). Default "
                        "is off (no midpoints added).");
  }
}

void GUI::options_sketch_add_arc_circle_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Sketch_add_seg_circle_arc);

  options_sketch_common_();
}

void GUI::options_sketch_add_square_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Sketch_add_square);

  options_sketch_common_();
  options_sketch_len_angle_hotkeys_();
}

void GUI::options_sketch_add_rectangle_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Sketch_add_rectangle);

  options_sketch_common_();
  options_sketch_len_angle_hotkeys_();
}

void GUI::options_sketch_add_rectangle_center_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Sketch_add_rectangle_center_pt);

  options_sketch_common_();
  options_sketch_len_angle_hotkeys_();
}

void GUI::options_sketch_add_circle_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Sketch_add_circle);

  options_sketch_common_();
  options_sketch_len_angle_hotkeys_();
}

void GUI::options_sketch_add_circle_three_pts_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Sketch_add_circle_3_pts);

  options_sketch_common_();
}

void GUI::options_sketch_add_slot_mode_()
{
  EZY_ASSERT(get_mode() == Mode::Sketch_add_slot);

  options_sketch_common_();
  options_sketch_len_angle_hotkeys_();
}

void GUI::options_orthographic_projection_()
{
  ImGui::Separator();
  ImGui::TextUnformatted("View");
  bool ortho = m_inspection_orthographic;
  if (ImGui::Checkbox("Orthographic projection", &ortho))
  {
    m_inspection_orthographic = ortho;
    save_occt_view_settings();
    m_view->apply_camera_projection();
  }
  if (ui_show_help(2))
  {
    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
      ImGui::TextDisabled("Use an orthographic camera in non-sketch modes (no perspective foreshortening).");
      ImGui::PopTextWrapPos();
      ImGui::EndTooltip();
    }
  }
}

void GUI::options_sketch_common_()
{
  ImGui::TextUnformatted(current_mode_description());
  options_doc_help_button_();

  ImGui::Separator();

  ImGui::TextUnformatted("Sketch options");
  if (ImGui::BeginTable("options_sketch_sketch", 2, k_options_table_flags))
  {
    options_table_setup_columns_(options_sketch_label_col_w_(), k_options_sketch_control_col_w);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    options_right_aligned_label_("Snap dist");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(140.0f);
    float snap_dist = float(Sketch_nodes::get_snap_dist());
    if (ImGui::InputFloat("##snap_dist", &snap_dist, 1.0f, 2.0f, "%.2f"))
      Sketch_nodes::set_snap_dist(snap_dist);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    options_right_aligned_label_("Snap guide mode");
    ImGui::TableSetColumnIndex(1);
    constexpr std::array<const char*, 3> k_snap_guide_mode_labels = {
        "Traditional",
        "Fullscreen",
        "Both",
    };
    int snap_mode = static_cast<int>(Sketch_nodes::get_snap_guide_mode());
    ImGui::SetNextItemWidth(140.0f);
    if (ImGui::BeginCombo("##snap_guide_mode", k_snap_guide_mode_labels[static_cast<size_t>(snap_mode)],
                          ImGuiComboFlags_HeightSmall))
    {
      for (int i = 0; i < static_cast<int>(k_snap_guide_mode_labels.size()); ++i)
        if (ImGui::Selectable(k_snap_guide_mode_labels[static_cast<size_t>(i)], i == snap_mode))
          Sketch_nodes::set_snap_guide_mode(static_cast<Sketch_nodes::Snap_guide_mode>(i));
      ImGui::EndCombo();
    }
    if (ui_show_help(2) && ImGui::IsItemHovered())
      ImGui::SetTooltip("Traditional: compact local snap marker.\n"
                        "Fullscreen: full-view crosshair/axis guides.\n"
                        "Both: show compact marker and fullscreen guides together.");

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    options_right_aligned_label_("All co-axial nodes");
    ImGui::TableSetColumnIndex(1);
    bool annotate_all = Sketch_nodes::get_annotate_all_coaxial_nodes();
    if (ImGui::Checkbox("##annotate_all_coaxial", &annotate_all))
      Sketch_nodes::set_annotate_all_coaxial_nodes(annotate_all);

    if (ui_show_help(2) && ImGui::IsItemHovered())
      ImGui::SetTooltip("When on (global mode): axis guide lines + markers for *all* nodes in the current\n"
                        "sketch and all other visible sketches (shows the full set of horizontal and\n"
                        "vertical co-axial alignments). When off (default): only closest node per active\n"
                        "axis is annotated (classic closest-relative behavior).");

    ImGui::EndTable();
  }
}

void GUI::options_sketch_len_angle_hotkeys_()
{
  if (ui_show_help(3))
  {
    ImGui::Separator();
    ImGui::TextUnformatted("Shortcuts");
    ImGui::TextWrapped("TAB: type edge length. Shift+TAB: type angle (degrees, CCW from +X).");
  }
}

float GUI::options_sketch_label_col_w_() const
{
  float sketch_label_col_w = ImGui::CalcTextSize("Snap dist").x;
  sketch_label_col_w       = std::max(sketch_label_col_w, ImGui::CalcTextSize("Snap guide mode").x);
  sketch_label_col_w       = std::max(sketch_label_col_w, ImGui::CalcTextSize("All co-axial nodes").x);
  if (get_mode() == Mode::Sketch_face_extrude)
  {
    sketch_label_col_w = std::max(sketch_label_col_w, ImGui::CalcTextSize("Both sides").x);
    sketch_label_col_w = std::max(sketch_label_col_w, ImGui::CalcTextSize("Material").x);
  }
  sketch_label_col_w += ImGui::GetStyle().CellPadding.x * 2.0f + 8.0f;
  return sketch_label_col_w;
}

void GUI::on_key_rotate_mode_(int key)
{
  const ScreenCoords screen_coords(dvec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y));

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
    if (Status s = m_view->shp_rotate().show_angle_edit(screen_coords); !s.is_ok())
      show_message(s.message());
    break;

  case GLFW_KEY_X:
    if (m_view->shp_rotate().get_rotation_axis() == Rotation_axis::X_axis)
      m_view->shp_rotate().set_rotation_axis(Rotation_axis::View_to_object);
    else
      m_view->shp_rotate().set_rotation_axis(Rotation_axis::X_axis);
    break;

  case GLFW_KEY_Y:
    if (m_view->shp_rotate().get_rotation_axis() == Rotation_axis::Y_axis)
      m_view->shp_rotate().set_rotation_axis(Rotation_axis::View_to_object);
    else
      m_view->shp_rotate().set_rotation_axis(Rotation_axis::Y_axis);
    break;

  case GLFW_KEY_Z:
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
  const ScreenCoords screen_coords(dvec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y));

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
