#include "gui.h"
#include "occt_view.h"

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
