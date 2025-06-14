#include "shp_rotate.h"

#include "geom.h"
#include "gui.h"
#include "imgui.h"
#include "occt_view.h"

Shp_rotate::Shp_rotate(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_rotate::rotate_selected(const ScreenCoords& screen_coords)
{
  CHK_RET(ensure_start_state_());

  std::optional<gp_Pnt> mouse_wc_pos = view().pt3d_on_plane(screen_coords, *m_rotate_pln);
  if (!mouse_wc_pos)
    return Status::user_error("Adjust view, cannot get point on plane.");

  if (m_center.IsEqual(*mouse_wc_pos, Precision::Confusion()))
    return Status::user_error("Move mouse, cannot get point on rotate vector.");

  if (!m_initial_mouse_pos)
    m_initial_mouse_pos = mouse_wc_pos;

  gp_Vec v0(m_center, *m_initial_mouse_pos);
  gp_Vec v1(m_center, *mouse_wc_pos);

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
  m_center = get_shape_bbox_center(m_shps[0]->Shape());

  if (!m_rotate_pln.has_value())
    m_rotate_pln = view().get_view_plane(m_center);

  return Status::ok();
}

void Shp_rotate::preview_rotate_()
{
  gp_Trsf rotation;
  gp_Ax1  rotation_axis(m_center, m_rotate_pln->Axis().Direction());
  rotation.SetRotation(rotation_axis, m_angle);

  for (const AIS_Shape_ptr& shape : m_shps)
  {
    // Apply to shape
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
    m_angle  = to_radians(new_angle);
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
  m_angle = 0;
  m_shps.clear();
  gui().set_mode(Mode::Normal);
}