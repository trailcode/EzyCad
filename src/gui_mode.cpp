// Tool mode switching, keyboard routing, and Options panel (mode-specific UI).

#include "gui.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string_view>
#include <vector>

#include "geom.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "modes.h"
#include "occt_view.h"
#include "sketch.h"

#include <GLFW/glfw3.h>

namespace {

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

}  // namespace

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

  (void) check;
  const auto itr = parent_modes.find(get_mode());
  EZY_ASSERT(itr != parent_modes.end());
  set_mode(itr->second);
}

void GUI::on_key(int key, int scancode, int action, int mods)
{
  (void) scancode;
  if (action == GLFW_PRESS)
  {
    const ScreenCoords screen_coords(glm::dvec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y));

    bool ctrl_pressed = (mods & GLFW_MOD_CONTROL) != 0;

#ifndef __EMSCRIPTEN__
    if (key == GLFW_KEY_F12)
    {
      m_show_lua_console = !m_show_lua_console;
      save_occt_view_settings();
      return;
    }
#else
    if (ctrl_pressed && (mods & GLFW_MOD_SHIFT) != 0 && key == GLFW_KEY_L)
    {
      m_show_lua_console = !m_show_lua_console;
      save_occt_view_settings();
      return;
    }
#endif

    if (ctrl_pressed)
    {
      switch (key)
      {
        case GLFW_KEY_N:
          m_view->new_file();
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

void GUI::options_()
{
  if (!m_show_options)
    return;

  if (!ImGui::Begin("Options", &m_show_options))
  {
    ImGui::End();
    return;
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

    if (get_mode() == Mode::Sketch_toggle_edge_dim)
    {
      constexpr std::array<const char*, 4> k_edge_dim_label_placement = {
          "Near first point",
          "Near second point",
          "Center on dimension line",
          "Automatic",
      };
      int h = m_edge_dim_label_h;
      if (ImGui::Combo("Length value placement##edge_dim_h", &h, k_edge_dim_label_placement.data(),
                       int(k_edge_dim_label_placement.size())))
      {
        m_edge_dim_label_h = h;
        save_occt_view_settings();
      }
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "Where to place the numeric value along the dimension line (near the first or second end, center, or auto).\n"
            "This does not flip which side of the edge the dimension sits on.\n"
            "When the sketch has filled faces, dimensions offset to the void side (point-in-face test).\n"
            "Otherwise the average node position is used as a rough inside reference.");
    }
  }
  else
  {
    const std::vector<std::string>& material_names = occt_material_combo_labels();
    int                             current_item   = int(m_view->get_default_material().Name());
    if (current_item < 0 || current_item >= static_cast<int>(material_names.size()))
      current_item = 0;

    if (ImGui::BeginCombo("Default Material##filter", material_names[static_cast<size_t>(current_item)].data(),
                          ImGuiComboFlags_WidthFitPreview | ImGuiComboFlags_HeightSmall))
    {
      for (int i = 0; i < static_cast<int>(material_names.size()); i++)
        if (ImGui::Selectable(material_names[static_cast<size_t>(i)].data(), current_item == i))
        {
          Graphic3d_MaterialAspect mat(static_cast<Graphic3d_NameOfMaterial>(i));
          m_view->set_default_material(mat);
        }

      ImGui::EndCombo();
    }
  }

  ImGui::End();
}

void GUI::options_normal_mode_()
{
  constexpr std::array<std::string_view, 9> c_names_TopAbs_ShapeEnum = {
      "COMPOUND",
      "COMPSOLID",
      "SOLID",
      "SHELL",
      "FACE",
      "WIRE",
      "EDGE",
      "VERTEX",
      "SHAPE"};

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

    static float revolve_angle = 360.0f;

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
  int current_mode = static_cast<int>(m_chamfer_mode);
  if (ImGui::Combo("Chamfer Mode", &current_mode, c_chamfer_mode_strs.data(), (int) c_chamfer_mode_strs.size()))
  {
    m_chamfer_mode = static_cast<Chamfer_mode>(current_mode);
    m_view->on_chamfer_mode();
  }

  static char chamfer_buf[64];
  const double scale = m_view->get_dimension_scale();
  const double chamfer_dist = m_view->shp_chamfer().get_chamfer_dist() / scale;

  ImGui::PushID("chamfer_dist_micron");
  const ImGuiID chamfer_input_id = ImGui::GetID("##micron");
  ImGuiContext* ctx = ImGui::GetCurrentContext();
  if (ctx && ctx->ActiveId != chamfer_input_id)
    format_double_trim_fraction(chamfer_buf, sizeof chamfer_buf, chamfer_dist, 6);

  ImGui::SetNextItemWidth(100.0f);
  constexpr ImGuiInputTextFlags k_dim_flags =
      ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsScientific;
  if (ImGui::InputText("##micron", chamfer_buf, sizeof chamfer_buf, k_dim_flags))
  {
    char* end = nullptr;
    const double p = std::strtod(chamfer_buf, &end);
    if (end != chamfer_buf)
    {
      while (*end == ' ' || *end == '\t')
        ++end;
      if (*end == '\0')
        m_view->shp_chamfer().set_chamfer_dist(p * scale);
    }
  }
  ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
  ImGui::TextUnformatted("Chamfer dist");
  ImGui::PopID();
}

void GUI::options_shape_fillet_mode_()
{
  int current_mode = static_cast<int>(m_fillet_mode);
  if (ImGui::Combo("Fillet Mode", &current_mode, c_fillet_mode_strs.data(), (int) c_fillet_mode_strs.size()))
  {
    m_fillet_mode = static_cast<Fillet_mode>(current_mode);
    m_view->on_fillet_mode();
  }

  static char fillet_buf[64];
  const double scale = m_view->get_dimension_scale();
  const double fillet_radius = m_view->shp_fillet().get_fillet_radius() / scale;

  ImGui::PushID("fillet_rad_micron");
  const ImGuiID fillet_input_id = ImGui::GetID("##micron");
  ImGuiContext* ctx = ImGui::GetCurrentContext();
  if (ctx && ctx->ActiveId != fillet_input_id)
    format_double_trim_fraction(fillet_buf, sizeof fillet_buf, fillet_radius, 6);

  ImGui::SetNextItemWidth(100.0f);
  constexpr ImGuiInputTextFlags k_dim_flags =
      ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsScientific;
  if (ImGui::InputText("##micron", fillet_buf, sizeof fillet_buf, k_dim_flags))
  {
    char* end = nullptr;
    const double p = std::strtod(fillet_buf, &end);
    if (end != fillet_buf)
    {
      while (*end == ' ' || *end == '\t')
        ++end;
      if (*end == '\0')
        m_view->shp_fillet().set_fillet_radius(p * scale);
    }
  }
  ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
  ImGui::TextUnformatted("Fillet radius");
  ImGui::PopID();
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
