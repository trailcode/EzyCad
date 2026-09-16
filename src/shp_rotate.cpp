#include "shp_rotate.h"

#include <AIS_InteractiveContext.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Builder.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <Precision.hxx>
#include <Quantity_Color.hxx>
#include <Quantity_NameOfColor.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <cmath>
#include <gp_Ax1.hxx>

#include "utl_geom.h"
#include "gui.h"
#include "gui_occt_view.h"
#include "shp_delta.h"
#include "utl.h"

Shp_rotate::Shp_rotate(Occt_view& view)
    : Shp_operation_base(view)
{
}

void Shp_rotate::begin(std::vector<Shp_ptr> shps)
{
  clear_all(m_angle, m_initial_mouse_pos, m_rotate_pln, m_captured_axis_dir, m_center);
  clear_rotation_vis_();
  set_operation_shps_(std::move(shps));
  if (!m_shps.empty())
    refresh_guides_();
}

Status Shp_rotate::rotate_selected(const ScreenCoords& screen_coords)
{
  CHK_RET(ensure_start_state_());

  const gp_Dir axis_dir = current_axis_dir_();
  const gp_Pln pln      = m_rotate_pln ? *m_rotate_pln : choose_rotate_pln_(axis_dir);

  std::optional<gp_Pnt> mouse_wc_pos = view().pt3d_on_plane(screen_coords, pln);
  if (!mouse_wc_pos)
    return Status::user_error("Adjust view, cannot get point on plane.");

  if (m_center->IsEqual(*mouse_wc_pos, Precision::Confusion()))
    return Status::user_error("Move mouse further from rotation center to start rotation.");

  if (!m_rotate_pln)
  {
    m_rotate_pln        = pln;
    m_captured_axis_dir = axis_dir;
  }

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
  refresh_guides_();
  ctx().UpdateCurrentViewer();
  return Status::ok();
}

Transform_axes Shp_rotate::current_axes_()
{
  return transform_axes_for(m_shps, gui().get_transform_space());
}

gp_Dir Shp_rotate::current_axis_dir_()
{
  if (m_captured_axis_dir)
    return *m_captured_axis_dir;

  EZY_ASSERT(m_center.has_value());
  switch (m_rotation_axis)
  {
    // clang-format off
  case Rotation_axis::X_axis: return current_axes_().x;
  case Rotation_axis::Y_axis: return current_axes_().y;
  case Rotation_axis::Z_axis: return current_axes_().z;
    // clang-format on
  case Rotation_axis::View_to_object:
    return view().get_view_plane(*m_center).Axis().Direction();
  }

  return gp_Dir(0.0, 0.0, 1.0);
}

gp_Pln Shp_rotate::choose_rotate_pln_(const gp_Dir& axis_dir)
{
  EZY_ASSERT(m_center.has_value());
  const gp_Pln view_pln = view().get_view_plane(*m_center);
  const gp_Pln axis_pln(*m_center, axis_dir);
  // Constrained X/Y/Z: use the plane perpendicular to the axis when the view faces it.
  // Edge-on views stay on the view plane so the mouse still has a usable lever.
  const bool use_axis_pln =
      m_rotation_axis != Rotation_axis::View_to_object && std::abs(view_pln.Axis().Direction().Dot(axis_dir)) >= 0.15;

  return use_axis_pln ? axis_pln : view_pln;
}

void Shp_rotate::capture_drag_frame_()
{
  if (m_rotate_pln)
    return;

  EZY_ASSERT(m_center.has_value());
  const gp_Dir axis_dir = current_axis_dir_();
  m_captured_axis_dir   = axis_dir;
  m_rotate_pln          = choose_rotate_pln_(axis_dir);
}

void Shp_rotate::refresh_guides_()
{
  if (m_shps.empty())
    return;

  m_center = current_axes_().origin;
  update_rotation_axis_();
  update_rotation_center_();
}

void Shp_rotate::update_rotation_axis_()
{
  EZY_ASSERT(m_center.has_value());

  const gp_Dir   axis_dir = current_axis_dir_();
  Quantity_Color axis_color;
  switch (m_rotation_axis)
  {
    // clang-format off
  case Rotation_axis::X_axis:         axis_color = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB); break;
  case Rotation_axis::Y_axis:         axis_color = Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB); break;
  case Rotation_axis::Z_axis:         axis_color = Quantity_Color(0.0, 0.0, 1.0, Quantity_TOC_RGB); break;
  case Rotation_axis::View_to_object: axis_color = Quantity_NOC_CYAN;                               break;
    // clang-format on
  }

  const gp_Vec axis_vec(axis_dir);
  const gp_Pnt p1 = m_center->Translated(axis_vec.Multiplied(-1000.0));
  const gp_Pnt p2 = m_center->Translated(axis_vec.Multiplied(1000.0));
  const TopoDS_Edge axis_edge = BRepBuilderAPI_MakeEdge(p1, p2).Edge();

  if (m_rotation_axis_vis)
  {
    m_rotation_axis_vis->Set(axis_edge);
    m_rotation_axis_vis->SetColor(axis_color);
    ctx().Redisplay(m_rotation_axis_vis, false);
  }
  else
  {
    m_rotation_axis_vis = new AIS_Shape(axis_edge);
    m_rotation_axis_vis->SetWidth(2.0);
    m_rotation_axis_vis->SetColor(axis_color);
    m_rotation_axis_vis->SetZLayer(Graphic3d_ZLayerId_Topmost);
    ctx().Display(m_rotation_axis_vis, AIS_WireFrame, -1, false);
    ctx().Deactivate(m_rotation_axis_vis);
  }
}

void Shp_rotate::update_rotation_center_()
{
  EZY_ASSERT(m_center.has_value());

  const Transform_axes axes = current_axes_();
  const double         arm  = 8.0;
  const gp_Pnt         o    = *m_center;
  TopoDS_Compound      cross;
  BRep_Builder().MakeCompound(cross);
  BRep_Builder().Add(cross, BRepBuilderAPI_MakeEdge(o.Translated(gp_Vec(axes.x) * -arm), o.Translated(gp_Vec(axes.x) * arm)).Edge());
  BRep_Builder().Add(cross, BRepBuilderAPI_MakeEdge(o.Translated(gp_Vec(axes.y) * -arm), o.Translated(gp_Vec(axes.y) * arm)).Edge());
  BRep_Builder().Add(cross, BRepBuilderAPI_MakeEdge(o.Translated(gp_Vec(axes.z) * -arm), o.Translated(gp_Vec(axes.z) * arm)).Edge());

  if (m_rotation_center_vis)
  {
    m_rotation_center_vis->Set(cross);
    ctx().Redisplay(m_rotation_center_vis, false);
  }
  else
  {
    m_rotation_center_vis = new AIS_Shape(cross);
    m_rotation_center_vis->SetWidth(3.0);
    m_rotation_center_vis->SetColor(Quantity_NOC_RED);
    m_rotation_center_vis->SetZLayer(Graphic3d_ZLayerId_Topmost);
    ctx().Display(m_rotation_center_vis, AIS_WireFrame, -1, false);
    ctx().Deactivate(m_rotation_center_vis);
  }
}

void Shp_rotate::preview_rotate_()
{
  EZY_ASSERT(m_center.has_value());

  gp_Trsf rotation;
  rotation.SetRotation(gp_Ax1(*m_center, current_axis_dir_()), m_angle);

  for (const Shp_ptr& shape : m_shps)
    shape->SetLocalTransformation(rotation);

  redisplay_operation_shps_after_transform_();
}

Status Shp_rotate::show_angle_edit(const ScreenCoords& screen_coords)
{
  CHK_RET(ensure_start_state_());

  auto angle_edit = [&, screen_coords](float new_angle, bool is_final)
  {
    m_angle = to_radians(new_angle);
    preview_rotate_();
    if (is_final)
      finalize();
  };

  gui().set_angle_edit(float(to_degrees(m_angle)), std::move(std::function<void(float, bool)>(angle_edit)));

  return Status::ok();
}

void Shp_rotate::finalize()
{
  if (m_shps.empty())
    return;

  std::vector<Shape_geom_delta::Geom_change> changes;
  changes.reserve(m_shps.size());
  for (const Shp_ptr& shape : m_shps)
    changes.push_back(Shape_geom_delta::Geom_change{shape->get_id(), shape->Shape(), {}, shape->get_frame(), {}});

  operation_shps_finalize_();

  for (Shape_geom_delta::Geom_change& ch : changes)
  {
    Shp_ptr shp = view().find_shape_by_id(ch.id);
    if (!shp.IsNull())
    {
      ch.after_geom  = shp->Shape();
      ch.after_frame = shp->get_frame();
    }
  }

  view().push_undo_delta(std::make_unique<Shape_geom_delta>(std::move(changes)));
  reset();
  restore_operation_selection_();
}

void Shp_rotate::cancel()
{
  operation_shps_cancel_();
  reset();
  restore_operation_selection_();
}

void Shp_rotate::reset()
{
  clear_all(m_angle, m_shps, m_initial_mouse_pos, m_rotate_pln, m_captured_axis_dir, m_center);
  clear_rotation_vis_();
  gui().set_mode(Mode::Normal);
}

void Shp_rotate::clear_rotation_vis_()
{
  if (!m_rotation_axis_vis.IsNull())
    ctx().Remove(m_rotation_axis_vis, false);

  if (!m_rotation_center_vis.IsNull())
    ctx().Remove(m_rotation_center_vis, false);
  clear_all(m_rotation_axis_vis, m_rotation_center_vis);
}

void Shp_rotate::set_rotation_axis(Rotation_axis axis)
{
  m_rotation_axis = axis;
  clear_all(m_initial_mouse_pos, m_rotate_pln, m_captured_axis_dir);
  if (m_shps.empty())
    return;

  refresh_guides_();
  if (std::abs(m_angle) > Precision::Confusion())
    preview_rotate_();

  ctx().UpdateCurrentViewer();
}

void Shp_rotate::on_transform_space_changed()
{
  if (m_shps.empty())
    return;

  clear_all(m_initial_mouse_pos, m_rotate_pln, m_captured_axis_dir);
  refresh_guides_();
  if (std::abs(m_angle) > Precision::Confusion())
    preview_rotate_();

  ctx().UpdateCurrentViewer();
}
