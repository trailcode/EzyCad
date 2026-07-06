#include "gui.h"
#include "utl_geom.h"
#include "gui_occt_view.h"

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
    ImGui::InputDouble("##box_origin_x", &m_add_box_origin.x, 0.0, 0.0, "%.3f");

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Y");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##box_origin_y", &m_add_box_origin.y, 0.0, 0.0, "%.3f");

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Z");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##box_origin_z", &m_add_box_origin.z, 0.0, 0.0, "%.3f");

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Width (X)");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##box_width", &m_add_box_size.x, 0.0, 0.0, "%.3f");

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Length (Y)");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##box_length", &m_add_box_size.y, 0.0, 0.0, "%.3f");

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Height (Z)");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##box_height", &m_add_box_size.z, 0.0, 0.0, "%.3f");

    ImGui::EndTable();
  }
  ImGui::Spacing();

  if (ImGui::Button("Add"))
  {
    if (m_add_box_size.x > 0 && m_add_box_size.y > 0 && m_add_box_size.z > 0)
    {
      const double scale = m_view->get_dimension_scale();
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

  ImGui::TextUnformatted("Values in display units.");
  ImGui::Spacing();
  if (ImGui::BeginTable("Add pyramid##table", 2, ImGuiTableFlags_SizingStretchProp))
  {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin X");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##pyramid_origin_x", &m_add_pyramid_origin.x, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Y");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##pyramid_origin_y", &m_add_pyramid_origin.y, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Z");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##pyramid_origin_z", &m_add_pyramid_origin.z, 0.0, 0.0, "%.3f");
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

  ImGui::TextUnformatted("Values in display units.");
  ImGui::Spacing();
  if (ImGui::BeginTable("Add sphere##table", 2, ImGuiTableFlags_SizingStretchProp))
  {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin X");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##sphere_origin_x", &m_add_sphere_origin.x, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Y");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##sphere_origin_y", &m_add_sphere_origin.y, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Z");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##sphere_origin_z", &m_add_sphere_origin.z, 0.0, 0.0, "%.3f");
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

  ImGui::TextUnformatted("Values in display units.");
  ImGui::Spacing();
  if (ImGui::BeginTable("Add cylinder##table", 2, ImGuiTableFlags_SizingStretchProp))
  {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin X");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##cyl_origin_x", &m_add_cylinder_origin.x, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Y");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##cyl_origin_y", &m_add_cylinder_origin.y, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Z");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##cyl_origin_z", &m_add_cylinder_origin.z, 0.0, 0.0, "%.3f");
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

  ImGui::TextUnformatted("Values in display units.");
  ImGui::Spacing();
  if (ImGui::BeginTable("Add cone##table", 2, ImGuiTableFlags_SizingStretchProp))
  {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin X");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##cone_origin_x", &m_add_cone_origin.x, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Y");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##cone_origin_y", &m_add_cone_origin.y, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Z");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##cone_origin_z", &m_add_cone_origin.z, 0.0, 0.0, "%.3f");
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

  ImGui::TextUnformatted("Values in display units.");
  ImGui::Spacing();
  if (ImGui::BeginTable("Add torus##table", 2, ImGuiTableFlags_SizingStretchProp))
  {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin X");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##torus_origin_x", &m_add_torus_origin.x, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Y");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##torus_origin_y", &m_add_torus_origin.y, 0.0, 0.0, "%.3f");
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Origin Z");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputDouble("##torus_origin_z", &m_add_torus_origin.z, 0.0, 0.0, "%.3f");
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

  ImGui::TextUnformatted("Plane and offset along its normal (display units).");
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

    const double scale = m_view->get_dimension_scale();
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
    const double scale = m_view->get_dimension_scale();
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
    const double scale = m_view->get_dimension_scale();
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
    const double scale = m_view->get_dimension_scale();
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
    const double scale = m_view->get_dimension_scale();
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
    const double scale = m_view->get_dimension_scale();
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
    const double scale = m_view->get_dimension_scale();
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