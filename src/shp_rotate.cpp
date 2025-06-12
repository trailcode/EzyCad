#include "shp_rotate.h"

#include "geom.h"
#include "gui.h"
#include "imgui.h"
#include "occt_view.h"

Shp_rotate::Shp_rotate(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_rotate::rotate_selected(const ScreenCoords& screen_coords)
{
  #if 0
  std::vector<ShapeBase_ptr> to_rotate = get_selected();
  if (to_rotate.empty())
    return Status::user_error("Select one or more shapes.");

  // Get the estimate of the center.
  // TODO consider all shapes.
  // Use the point of the first selected object.
  const gp_Pnt center = get_shape_bbox_center(to_rotate[0]->Shape());

  if (!m_rotate_pln.has_value())
    m_rotate_pln = view().get_view_plane(center);

  std::optional<gp_Pnt> mouse_wc_pos = view().pt3d_on_plane(screen_coords, *m_rotate_pln);
  if (!mouse_wc_pos)
    return Status::user_error("Adjust view, cannot get point on plane.");

  if (center.IsEqual(*mouse_wc_pos, Precision::Confusion()))
    return Status::user_error("Move mouse, cannot get point on rotate vector.");

  if (!m_initial_mouse_pos)
    m_initial_mouse_pos = mouse_wc_pos;

  gp_Vec v0(center, *m_initial_mouse_pos);
  gp_Vec v1(center, *mouse_wc_pos);

  gp_Vec v0_proj = project_onto_plane(v0, *m_rotate_pln);
  gp_Vec v1_proj = project_onto_plane(v1, *m_rotate_pln);

  double angle = v0_proj.Angle(v1_proj);
  gp_Vec cross = v0_proj.Crossed(v1_proj);
  if (cross.Dot(gp_Vec(m_rotate_pln->Axis().Direction())) < 0)
    angle = -angle;

  gp_Trsf rotation;
  gp_Ax1  rotation_axis(center, m_rotate_pln->Axis().Direction());
  rotation.SetRotation(rotation_axis, angle);

  for (const AIS_Shape_ptr& shape : to_rotate)
  {
    // Apply to shape
    shape->SetLocalTransformation(rotation);
    ctx().Redisplay(shape, true);
  }
  #endif
  return Status::ok();
}

void Shp_rotate::show_dist_edit(const ScreenCoords& screen_coords)
{
}

void Shp_rotate::finalize_rotate_selected()
{
  int hi = 0;
}

void Shp_rotate::cancel_rotate_selected()
{

}