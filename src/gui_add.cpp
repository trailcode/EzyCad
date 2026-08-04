#include "gui.h"
#include "utl_geom.h"
#include "gui_occt_view.h"

namespace
{
void table_row_input_double_(const char* label, const char* id, double* value);
} // namespace

void GUI::add_box_dialog_()
{
  if (m_open_add_box_popup)
  {
    ImGui::OpenPopup("Add box");
    m_open_add_box_popup = false;
  }

  if (!ImGui::BeginPopupModal("Add box", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    return;

  ImGui::Text("Values in project units (%s).", m_view->project_unit_suffix());
  ImGui::Spacing();

  if (ImGui::BeginTable("Add box##table", 2, ImGuiTableFlags_SizingStretchProp))
  {
    table_row_input_double_("Origin X", "##box_origin_x", &m_add_box_origin.x);
    table_row_input_double_("Origin Y", "##box_origin_y", &m_add_box_origin.y);
    table_row_input_double_("Origin Z", "##box_origin_z", &m_add_box_origin.z);
    table_row_input_double_("Width (X)", "##box_width", &m_add_box_size.x);
    table_row_input_double_("Length (Y)", "##box_length", &m_add_box_size.y);
    table_row_input_double_("Height (Z)", "##box_height", &m_add_box_size.z);
    ImGui::EndTable();
  }
  ImGui::Spacing();

  if (ImGui::Button("Add"))
  {
    if (m_add_box_size.x > 0 && m_add_box_size.y > 0 && m_add_box_size.z > 0)
    {
      const double scale = m_view->get_display_to_model_scale();
      m_view->add_box(m_add_box_origin.x * scale, m_add_box_origin.y * scale, m_add_box_origin.z * scale,
                      m_add_box_size.x * scale, m_add_box_size.y * scale, m_add_box_size.z * scale);

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

  ImGui::Text("Values in project units (%s).", m_view->project_unit_suffix());
  ImGui::Spacing();
  if (ImGui::BeginTable("Add pyramid##table", 2, ImGuiTableFlags_SizingStretchProp))
  {
    table_row_input_double_("Origin X", "##pyramid_origin_x", &m_add_pyramid_origin.x);
    table_row_input_double_("Origin Y", "##pyramid_origin_y", &m_add_pyramid_origin.y);
    table_row_input_double_("Origin Z", "##pyramid_origin_z", &m_add_pyramid_origin.z);
    table_row_input_double_("Side (base & height)", "##pyramid_side", &m_add_pyramid_side);
    ImGui::EndTable();
  }
  ImGui::Spacing();
  if (ImGui::Button("Add") && m_add_pyramid_side > 0)
  {
    const double scale = m_view->get_display_to_model_scale();
    m_view->add_pyramid(m_add_pyramid_origin.x * scale, m_add_pyramid_origin.y * scale, m_add_pyramid_origin.z * scale,
                        m_add_pyramid_side * scale);

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

  ImGui::Text("Values in project units (%s).", m_view->project_unit_suffix());
  ImGui::Spacing();
  if (ImGui::BeginTable("Add sphere##table", 2, ImGuiTableFlags_SizingStretchProp))
  {
    table_row_input_double_("Origin X", "##sphere_origin_x", &m_add_sphere_origin.x);
    table_row_input_double_("Origin Y", "##sphere_origin_y", &m_add_sphere_origin.y);
    table_row_input_double_("Origin Z", "##sphere_origin_z", &m_add_sphere_origin.z);
    table_row_input_double_("Radius", "##sphere_radius", &m_add_sphere_radius);
    ImGui::EndTable();
  }

  ImGui::Spacing();
  if (ImGui::Button("Add") && m_add_sphere_radius > 0)
  {
    const double scale = m_view->get_display_to_model_scale();
    m_view->add_sphere(m_add_sphere_origin.x * scale, m_add_sphere_origin.y * scale, m_add_sphere_origin.z * scale,
                       m_add_sphere_radius * scale);

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

  ImGui::Text("Values in project units (%s).", m_view->project_unit_suffix());
  ImGui::Spacing();
  if (ImGui::BeginTable("Add cylinder##table", 2, ImGuiTableFlags_SizingStretchProp))
  {
    table_row_input_double_("Origin X", "##cyl_origin_x", &m_add_cylinder_origin.x);
    table_row_input_double_("Origin Y", "##cyl_origin_y", &m_add_cylinder_origin.y);
    table_row_input_double_("Origin Z", "##cyl_origin_z", &m_add_cylinder_origin.z);
    table_row_input_double_("Radius", "##cyl_radius", &m_add_cylinder_radius);
    table_row_input_double_("Height", "##cyl_height", &m_add_cylinder_height);
    ImGui::EndTable();
  }

  ImGui::Spacing();
  if (ImGui::Button("Add") && m_add_cylinder_radius > 0 && m_add_cylinder_height > 0)
  {
    const double scale = m_view->get_display_to_model_scale();
    m_view->add_cylinder(m_add_cylinder_origin.x * scale, m_add_cylinder_origin.y * scale, m_add_cylinder_origin.z * scale,
                         m_add_cylinder_radius * scale, m_add_cylinder_height * scale);

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

  ImGui::Text("Values in project units (%s).", m_view->project_unit_suffix());
  ImGui::Spacing();
  if (ImGui::BeginTable("Add cone##table", 2, ImGuiTableFlags_SizingStretchProp))
  {
    table_row_input_double_("Origin X", "##cone_origin_x", &m_add_cone_origin.x);
    table_row_input_double_("Origin Y", "##cone_origin_y", &m_add_cone_origin.y);
    table_row_input_double_("Origin Z", "##cone_origin_z", &m_add_cone_origin.z);
    table_row_input_double_("Base radius (R1)", "##cone_R1", &m_add_cone_R1);
    table_row_input_double_("Top radius (R2)", "##cone_R2", &m_add_cone_R2);
    table_row_input_double_("Height", "##cone_height", &m_add_cone_height);
    ImGui::EndTable();
  }

  ImGui::Spacing();
  if (ImGui::Button("Add") && m_add_cone_R1 >= 0 && m_add_cone_R2 >= 0 && m_add_cone_height > 0)
  {
    const double scale = m_view->get_display_to_model_scale();
    m_view->add_cone(m_add_cone_origin.x * scale, m_add_cone_origin.y * scale, m_add_cone_origin.z * scale,
                     m_add_cone_R1 * scale, m_add_cone_R2 * scale, m_add_cone_height * scale);

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

  ImGui::Text("Values in project units (%s).", m_view->project_unit_suffix());
  ImGui::Spacing();
  if (ImGui::BeginTable("Add torus##table", 2, ImGuiTableFlags_SizingStretchProp))
  {
    table_row_input_double_("Origin X", "##torus_origin_x", &m_add_torus_origin.x);
    table_row_input_double_("Origin Y", "##torus_origin_y", &m_add_torus_origin.y);
    table_row_input_double_("Origin Z", "##torus_origin_z", &m_add_torus_origin.z);
    table_row_input_double_("Major radius (R1)", "##torus_R1", &m_add_torus_R1);
    table_row_input_double_("Minor radius (R2)", "##torus_R2", &m_add_torus_R2);
    ImGui::EndTable();
  }

  ImGui::Spacing();
  if (ImGui::Button("Add") && m_add_torus_R1 > 0 && m_add_torus_R2 > 0)
  {
    const double scale = m_view->get_display_to_model_scale();
    m_view->add_torus(m_add_torus_origin.x * scale, m_add_torus_origin.y * scale, m_add_torus_origin.z * scale,
                      m_add_torus_R1 * scale, m_add_torus_R2 * scale);

    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel"))
    ImGui::CloseCurrentPopup();

  ImGui::EndPopup();
}

void GUI::add_sketch_dialog_()
{
  if (m_open_add_sketch_popup)
  {
    ImGui::OpenPopup("New sketch");
    m_open_add_sketch_popup = false;
  }

  if (!ImGui::BeginPopupModal("New sketch", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    return;

  ImGui::Text("Plane and offset along its normal (project units, %s).", m_view->project_unit_suffix());
  ImGui::Spacing();

  ImGui::TextUnformatted("Reference plane");
  if (ImGui::RadioButton("XY", m_new_sketch_plane == 0))
    m_new_sketch_plane = 0;

  ImGui::SameLine();
  if (ImGui::RadioButton("XZ", m_new_sketch_plane == 1))
    m_new_sketch_plane = 1;

  ImGui::SameLine();
  if (ImGui::RadioButton("YZ", m_new_sketch_plane == 2))
    m_new_sketch_plane = 2;

  ImGui::Spacing();
  ImGui::SetNextItemWidth(160.f);
  ImGui::InputDouble("Offset along normal", &m_new_sketch_offset, 0.0, 0.0, "%.6g");

  ImGui::Spacing();
  if (ImGui::Button("Create"))
  {
    Sketch_ref_plane plane = Sketch_ref_plane::XY;
    const char*      base  = "Sketch_xy";
    switch (m_new_sketch_plane)
    {
    case 1:
      plane = Sketch_ref_plane::XZ;
      base  = "Sketch_xz";
      break;

    case 2:
      plane = Sketch_ref_plane::YZ;
      base  = "Sketch_yz";
      break;

    default:
      break;
    }

    const double scale = m_view->get_display_to_model_scale();
    const gp_Pln pln   = sketch_reference_plane(plane, m_new_sketch_offset * scale);
    m_view->add_sketch(pln, base);
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel"))
    ImGui::CloseCurrentPopup();

  ImGui::EndPopup();
}

void GUI::add_menu_items_()
{
  ImGui::Separator();
  if (ImGui::MenuItem("New sketch..."))
  {
    m_new_sketch_plane      = 0;
    m_new_sketch_offset     = 0.0;
    m_open_add_sketch_popup = true;
  }

  ImGui::Separator();
  if (ImGui::MenuItem("Add box"))
  {
    const double scale = m_view->get_display_to_model_scale();
    m_view->add_box(0, 0, 0, scale, scale, scale);
  }

  if (ui_show_feature(3) && ImGui::MenuItem("Add box_prms"))
  {
    m_add_box_origin     = glm::dvec3(0.0, 0.0, 0.0);
    m_add_box_size       = glm::dvec3(1.0, 1.0, 1.0);
    m_open_add_box_popup = true;
  }

  if (ImGui::MenuItem("Add pyramid"))
  {
    const double scale = m_view->get_display_to_model_scale();
    m_view->add_pyramid(0, 0, 0, scale);
  }

  if (ui_show_feature(3) && ImGui::MenuItem("Add pyramid_prms"))
  {
    m_add_pyramid_origin     = glm::dvec3(0.0, 0.0, 0.0);
    m_add_pyramid_side       = 1.0;
    m_open_add_pyramid_popup = true;
  }

  if (ImGui::MenuItem("Add sphere"))
  {
    const double scale = m_view->get_display_to_model_scale();
    m_view->add_sphere(0, 0, 0, scale);
  }

  if (ui_show_feature(3) && ImGui::MenuItem("Add sphere_prms"))
  {
    m_add_sphere_origin     = glm::dvec3(0.0, 0.0, 0.0);
    m_add_sphere_radius     = 1.0;
    m_open_add_sphere_popup = true;
  }

  if (ImGui::MenuItem("Add cylinder"))
  {
    const double scale = m_view->get_display_to_model_scale();
    m_view->add_cylinder(0, 0, 0, scale, scale);
  }

  if (ui_show_feature(3) && ImGui::MenuItem("Add cylinder_prms"))
  {
    m_add_cylinder_origin = glm::dvec3(0.0, 0.0, 0.0);
    m_add_cylinder_radius = m_add_cylinder_height = 1.0;
    m_open_add_cylinder_popup                     = true;
  }

  if (ImGui::MenuItem("Add cone"))
  {
    const double scale = m_view->get_display_to_model_scale();
    m_view->add_cone(0, 0, 0, scale, 0.0, scale);
  }

  if (ui_show_feature(3) && ImGui::MenuItem("Add cone_prms"))
  {
    m_add_cone_origin     = glm::dvec3(0.0, 0.0, 0.0);
    m_add_cone_R1         = 1.0;
    m_add_cone_R2         = 0.0;
    m_add_cone_height     = 1.0;
    m_open_add_cone_popup = true;
  }

  if (ImGui::MenuItem("Add torus"))
  {
    const double scale = m_view->get_display_to_model_scale();
    m_view->add_torus(0, 0, 0, scale, scale / 2.0);
  }

  if (ui_show_feature(3) && ImGui::MenuItem("Add torus_prms"))
  {
    m_add_torus_origin     = glm::dvec3(0.0, 0.0, 0.0);
    m_add_torus_R1         = 1.0;
    m_add_torus_R2         = 0.5;
    m_open_add_torus_popup = true;
  }
}

namespace
{
void table_row_input_double_(const char* label, const char* id, double* value)
{
  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::TextUnformatted(label);
  ImGui::TableSetColumnIndex(1);
  ImGui::SetNextItemWidth(-1);
  ImGui::InputDouble(id, value, 0.0, 0.0, "%.3f");
}
} // namespace
