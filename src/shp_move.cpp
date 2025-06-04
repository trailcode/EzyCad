#include "shp_move.h"

#include "geom.h"
#include "gui.h"
#include "occt_view.h"

Shp_move::Shp_move(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_move::move_selected(const ScreenCoords& screen_coords)
{
  const std::vector<AIS_Shape_ptr> selected = get_selected();
  if (selected.empty())
    return Status::user_error("Select one or more shapes.");

  if (!m_move_pln.has_value())
  {
    // Get the estimate of the center.
    // TODO consider all shapes.
    const gp_Pnt center = get_shape_bbox_center(selected[0]->Shape());
    m_move_pln          = view().get_view_plane(center);
  }

  DO_ASSERT(m_move_pln.has_value());

  std::optional<gp_Pnt> pos = view().pt3d_on_plane(screen_coords, *m_move_pln);
  if (!pos)
    return Status::user_error("Adjust view, cannot get point on plane.");

  // Use the point of the first selected object.
  gp_Pnt center = get_shape_bbox_center(selected.front()->Shape());

  bool no_axis_constraints = !m_opts.constrain_axis_x && !m_opts.constrain_axis_y && !m_opts.constrain_axis_z;

  for (const AIS_Shape_ptr& shape : selected)
  {
    // gp_Pnt center = get_shape_bbox_center(shape->Shape());
    gp_Trsf translation;

    translation.SetTranslation(gp_Vec(no_axis_constraints || m_opts.constrain_axis_x ? pos->X() - center.X() : 0,
                                      no_axis_constraints || m_opts.constrain_axis_y ? pos->Y() - center.Y() : 0,
                                      no_axis_constraints || m_opts.constrain_axis_z ? pos->Z() - center.Z() : 0));

    shape->SetLocalTransformation(translation);
    ctx().Redisplay(shape, true);
  }

  return Status::ok();
}

void Shp_move::finalize_move_selected()
{
  std::vector<AIS_Shape_ptr> selected = get_selected();
  if (selected.empty())
    return;

  for (AIS_Shape_ptr& shape : selected)
    view().bake_transform_into_geometry(shape);

  m_opts = {};  // Reset options;
  gui().set_mode(Mode::Normal);
}

void Shp_move::cancel_move_selected()
{
  for (AIS_Shape_ptr& shape : get_selected())
    shape->ResetTransformation();

  m_opts = {};  // Reset options;
  gui().set_mode(Mode::Normal);
}

Move_options& Shp_move::get_opts()
{
  return m_opts;
}