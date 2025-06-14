#include "shp_rotate.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>

#include "geom.h"
#include "gui.h"
#include "imgui.h"
#include "occt_view.h"

Shp_rotate::Shp_rotate(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_rotate::rotate_selected(const ScreenCoords& screen_coords)
{
  CHK_RET(ensure_start_state_());

  if (m_opts.custom_center)
  {
    std::optional<gp_Pnt> mouse_wc_pos = view().pt3d_on_plane(screen_coords, *m_rotate_pln);
    if (!mouse_wc_pos)
      return Status::user_error("Adjust view, cannot get point on plane.");

    m_center = *mouse_wc_pos;
    update_rotation_center_();
    return Status::ok();
  }

  if (!m_center)
    return Status::user_error("No rotation center set");

  std::optional<gp_Pnt> mouse_wc_pos = view().pt3d_on_plane(screen_coords, *m_rotate_pln);
  if (!mouse_wc_pos)
    return Status::user_error("Adjust view, cannot get point on plane.");

  if (m_center->IsEqual(*mouse_wc_pos, Precision::Confusion()))
    return Status::user_error("Move mouse, cannot get point on rotate vector.");

  if (!m_initial_mouse_pos)
    m_initial_mouse_pos = mouse_wc_pos;

  gp_Vec v0(*m_center, *m_initial_mouse_pos);
  gp_Vec v1(*m_center, *mouse_wc_pos);

  gp_Vec v0_proj = project_onto_plane(v0, *m_rotate_pln);
  gp_Vec v1_proj = project_onto_plane(v1, *m_rotate_pln);

  m_angle      = v0_proj.Angle(v1_proj);
  gp_Vec cross = v0_proj.Crossed(v1_proj);
  if (cross.Dot(gp_Vec(m_rotate_pln->Axis().Direction())) < 0)
    m_angle = -m_angle;

  preview_rotate_();

  return Status::ok();
}

Status Shp_rotate::ensure_start_state_()
{
  CHK_RET(ensure_operation_shps_());

  // Get the estimate of the center.
  // TODO consider all shapes.
  // Use the point of the first selected object.
  if (!m_center.has_value())
    m_center = get_shape_bbox_center(m_shps[0]->Shape());

  if (!m_rotate_pln.has_value())
  {
    if (m_opts.constr_axis_x || m_opts.constr_axis_y || m_opts.constr_axis_z)
    {
      // When an axis is constrained, create a rotation plane perpendicular to that axis
      gp_Dir axis_dir;
      if (m_opts.constr_axis_x)
        axis_dir = gp_Dir(1, 0, 0);
      else if (m_opts.constr_axis_y)
        axis_dir = gp_Dir(0, 1, 0);
      else if (m_opts.constr_axis_z)
        axis_dir = gp_Dir(0, 0, 1);

      // Create a plane perpendicular to the axis
      // We'll use the view's up direction to help orient the plane
      gp_Dir view_up = view().get_view_plane(*m_center).Axis().Direction();
      gp_Dir plane_normal = axis_dir.Crossed(view_up);
      if (plane_normal.Magnitude() < Precision::Confusion())
      {
        // If view up is parallel to axis, use a different reference direction
        plane_normal = axis_dir.Crossed(gp_Dir(0, 0, 1));
      }
      plane_normal.Normalize();
      
      m_rotate_pln = gp_Pln(*m_center, plane_normal);
    }
    else
    {
      m_rotate_pln = view().get_view_plane(*m_center);
    }
  }

  update_rotation_axis_();
  update_rotation_center_();

  return Status::ok();
}

void Shp_rotate::update_rotation_axis_()
{
  DO_ASSERT(m_center.has_value());
  DO_ASSERT(m_rotate_pln.has_value());

  gp_Dir axis_dir;
  if (m_opts.constr_axis_x)
    axis_dir = gp_Dir(1, 0, 0);
  else if (m_opts.constr_axis_y)
    axis_dir = gp_Dir(0, 1, 0);
  else if (m_opts.constr_axis_z)
    axis_dir = gp_Dir(0, 0, 1);
  else
    return;

  // Create a line representing the rotation axis
  gp_Lin axis(*m_center, axis_dir);

  // Create points along the line by moving from center in both directions
  gp_Vec axis_vec(axis_dir);
  gp_Pnt p1 = m_center->Translated(axis_vec.Multiplied(-1000.0));
  gp_Pnt p2 = m_center->Translated(axis_vec.Multiplied(1000.0));

  TopoDS_Edge axis_edge = BRepBuilderAPI_MakeEdge(p1, p2).Edge();

  if (m_rotation_axis_vis)
  {
    m_rotation_axis_vis->Set(axis_edge);
    ctx().Redisplay(m_rotation_axis_vis, true);
  }
  else
  {
    m_rotation_axis_vis = new AIS_Shape(axis_edge);
    m_rotation_axis_vis->SetWidth(2.0);
    m_rotation_axis_vis->SetColor(Quantity_NOC_YELLOW);
    ctx().Display(m_rotation_axis_vis, true);
  }
}

void Shp_rotate::update_rotation_center_()
{
  DO_ASSERT(m_center.has_value());

  // TODO cannot see the point unless the shape is rendered as wireframe
  // Create a point representing the rotation center
  TopoDS_Vertex center_vertex = BRepBuilderAPI_MakeVertex(*m_center).Vertex();

  if (m_rotation_center_vis)
  {
    m_rotation_center_vis->Set(center_vertex);
    ctx().Redisplay(m_rotation_center_vis, true);
  }
  else
  {
    m_rotation_center_vis = new AIS_Shape(center_vertex);
    m_rotation_center_vis->SetWidth(3.0);
    m_rotation_center_vis->SetColor(Quantity_NOC_RED);
    ctx().Display(m_rotation_center_vis, true);
  }
}

void Shp_rotate::preview_rotate_()
{
  DO_ASSERT(m_center.has_value());
  DO_ASSERT(m_rotate_pln.has_value());

  gp_Dir axis_dir;
  if (m_opts.constr_axis_x)
    axis_dir = gp_Dir(1, 0, 0);
  else if (m_opts.constr_axis_y)
    axis_dir = gp_Dir(0, 1, 0);
  else if (m_opts.constr_axis_z)
    axis_dir = gp_Dir(0, 0, 1);
  else
    axis_dir = m_rotate_pln->Axis().Direction();

  // Create rotation transformation
  gp_Trsf rotation;
  gp_Ax1 rotation_axis(*m_center, axis_dir);
  rotation.SetRotation(rotation_axis, m_angle);

  // Apply rotation to shapes
  for (const AIS_Shape_ptr& shape : m_shps)
  {
    shape->SetLocalTransformation(rotation);
    ctx().Redisplay(shape, true);
  }
}

Status Shp_rotate::show_angle_edit(const ScreenCoords& screen_coords)
{
  // In case `tab` was pressed without moving the mouse
  CHK_RET(ensure_start_state_());

  auto angle_edit = [&, screen_coords](float new_angle, bool is_final)
  {
    m_angle = to_radians(new_angle);
    preview_rotate_();
    if (is_final)
      finalize();
  };
  gui().set_dist_edit(float(to_degrees(m_angle)), std::move(std::function<void(float, bool)>(angle_edit)));
  return Status::ok();
}

void Shp_rotate::finalize()
{
  if (m_shps.empty())
    // If the rotate tool is activated and no shapes are selected,
    // then we do not want to call post_opts_() because they could
    // be selected while in rotate mode.
    return;

  operation_shps_finalize_();
  post_opts_();
}

void Shp_rotate::cancel()
{
  operation_shps_cancel_();
  post_opts_();
}

void Shp_rotate::post_opts_()
{
  // Reset state
  clear_all(m_angle, m_shps, m_initial_mouse_pos, m_rotate_pln, m_center);
  
  if (m_rotation_axis_vis)
  {
    ctx().Remove(m_rotation_axis_vis, false);
    m_rotation_axis_vis = nullptr;
  }
  
  if (m_rotation_center_vis)
  {
    ctx().Remove(m_rotation_center_vis, false);
    m_rotation_center_vis = nullptr;
  }
  
  gui().set_mode(Mode::Normal);
}

Rotate_options& Shp_rotate::get_opts()
{
  return m_opts;
}