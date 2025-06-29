#include "shp_rotate.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>

#include "geom.h"
#include "gui.h"
#include "utl.h"
#include "occt_view.h"

Shp_rotate::Shp_rotate(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_rotate::rotate_selected(const ScreenCoords& screen_coords)
{
  CHK_RET(ensure_start_state_());

  std::optional<gp_Pnt> mouse_wc_pos = view().pt3d_on_plane(screen_coords, *m_rotate_pln);
  if (!mouse_wc_pos)
    return Status::user_error("Adjust view, cannot get point on plane.");

  if (m_center->IsEqual(*mouse_wc_pos, Precision::Confusion()))
    return Status::user_error("Move mouse further from rotation center to start rotation.");

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
    m_rotate_pln = view().get_view_plane(*m_center);

  update_rotation_axis_();
  update_rotation_center_();

  return Status::ok();
}

void Shp_rotate::update_rotation_axis_()
{
  EZY_ASSERT(m_center.has_value());
  EZY_ASSERT(m_rotate_pln.has_value());

  // If View_to_object is set, remove the axis visualization
  if (m_rotation_axis == Rotation_axis::View_to_object)
  {
    if (m_rotation_axis_vis)
    {
      ctx().Remove(m_rotation_axis_vis, false);
      m_rotation_axis_vis = nullptr;
    }
    return;
  }

  gp_Dir         axis_dir;
  Quantity_Color axis_color;
  switch (m_rotation_axis)
  {
    case Rotation_axis::X_axis:
      axis_dir   = gp_Dir(1, 0, 0);
      axis_color = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB);  // Red
      break;
    case Rotation_axis::Y_axis:
      axis_dir   = gp_Dir(0, 1, 0);
      axis_color = Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB);  // Green
      break;
    case Rotation_axis::Z_axis:
      axis_dir   = gp_Dir(0, 0, 1);
      axis_color = Quantity_Color(0.0, 0.0, 1.0, Quantity_TOC_RGB);  // Blue
      break;
    case Rotation_axis::View_to_object:
      return;  // Already handled above
  }

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
    m_rotation_axis_vis->SetColor(axis_color);
    ctx().Redisplay(m_rotation_axis_vis, true);
  }
  else
  {
    m_rotation_axis_vis = new AIS_Shape(axis_edge);
    m_rotation_axis_vis->SetWidth(2.0);
    m_rotation_axis_vis->SetColor(axis_color);
    ctx().Display(m_rotation_axis_vis, true);
  }
}

void Shp_rotate::update_rotation_center_()
{
  EZY_ASSERT(m_center.has_value());

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
  EZY_ASSERT(m_center.has_value());
  EZY_ASSERT(m_rotate_pln.has_value());

  gp_Dir axis_dir;
  switch (m_rotation_axis)
  {
    case Rotation_axis::X_axis:
      axis_dir = gp_Dir(1, 0, 0);
      break;
    case Rotation_axis::Y_axis:
      axis_dir = gp_Dir(0, 1, 0);
      break;
    case Rotation_axis::Z_axis:
      axis_dir = gp_Dir(0, 0, 1);
      break;
    case Rotation_axis::View_to_object:
      axis_dir = m_rotate_pln->Axis().Direction();
      break;
  }

  // Create rotation transformation
  gp_Trsf rotation;
  gp_Ax1  rotation_axis(*m_center, axis_dir);
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
  gui().set_dist_edit(
      float(to_degrees(m_angle)),
      std::move(std::function<void(float, bool)>(angle_edit)));

  return Status::ok();
}

void Shp_rotate::finalize()
{
  if (m_shps.empty())
    return;

  operation_shps_finalize_();
  reset();
}

void Shp_rotate::cancel()
{
  operation_shps_cancel_();
  reset();
}

void Shp_rotate::reset()
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

void Shp_rotate::set_rotation_axis(Rotation_axis axis)
{
  m_rotation_axis = axis;
  update_rotation_axis_();
}