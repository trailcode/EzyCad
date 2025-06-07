#include "shp_move.h"

#include "geom.h"
#include "gui.h"
#include "occt_view.h"

Shp_move::Shp_move(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_move::move_selected(const ScreenCoords& screen_coords)
{
  std::vector<AIS_Shape_ptr> to_move = get_selected();
  if (to_move.empty())
    return Status::user_error("Select one or more shapes.");

  if (!m_move_pln.has_value())
  {
    // Get the estimate of the center.
    // TODO consider all shapes.
    const gp_Pnt center = get_shape_bbox_center(to_move[0]->Shape());
    m_move_pln          = view().get_view_plane(center);
  }

  DO_ASSERT(m_move_pln.has_value());

  std::optional<gp_Pnt> mouse_wc_pos = view().pt3d_on_plane(screen_coords, *m_move_pln);
  if (!mouse_wc_pos)
    return Status::user_error("Adjust view, cannot get point on plane.");

  // Use the point of the first selected object.
  gp_Pnt center = get_shape_bbox_center(to_move.front()->Shape());

  bool no_axis_constraints = !m_opts.constr_axis_x && !m_opts.constr_axis_y && !m_opts.constr_axis_z;

  for (const AIS_Shape_ptr& shape : to_move)
  {
    if (m_delta.override_x.has_value())
      m_delta.delta.SetX(*m_delta.override_x);
    else
      m_delta.delta.SetX(no_axis_constraints || m_opts.constr_axis_x ? mouse_wc_pos->X() - center.X() : 0);

    if (m_delta.override_y.has_value())
      m_delta.delta.SetY(*m_delta.override_y);
    else
      m_delta.delta.SetY(no_axis_constraints || m_opts.constr_axis_y ? mouse_wc_pos->Y() - center.Y() : 0);

    if (m_delta.override_z.has_value())
      m_delta.delta.SetZ(*m_delta.override_z);
    else
      m_delta.delta.SetZ(no_axis_constraints || m_opts.constr_axis_z ? mouse_wc_pos->Z() - center.Z() : 0);

    gp_Trsf translation;
    translation.SetTranslation(gp_Vec(m_delta.delta));

    shape->SetLocalTransformation(translation);
    ctx().Redisplay(shape, true);
  }

  return Status::ok();
}

void Shp_move::show_dist_edit(const ScreenCoords& screen_coords)
{
  bool no_axis_constraints = !m_opts.constr_axis_x && !m_opts.constr_axis_y && !m_opts.constr_axis_z;

  m_dist_axis_x = [&, screen_coords](float new_dist, bool is_final)
  {
    m_delta.override_x = new_dist * view().get_dimension_scale();
    move_selected(screen_coords);
    if (is_final)
      if (!check_finalize_())
      {
        // if (!m_delta.override_y.has_value() && (no_axis_constraints || m_opts.constr_axis_y))
        // gui().set_dist_edit(float(m_delta.delta.Y() / view().get_dimension_scale()), std::move(m_dist_axis_y), screen_coords);

        int hi = 0;
      }
  };

  m_dist_axis_y = [&, screen_coords](float new_dist, bool is_final)
  {
    m_delta.override_y = new_dist * view().get_dimension_scale();
    move_selected(screen_coords);
    if (is_final)
      check_finalize_();
  };

  m_dist_axis_z = [&, screen_coords](float new_dist, bool is_final)
  {
    m_delta.override_z = new_dist * view().get_dimension_scale();
    move_selected(screen_coords);
    if (is_final)
      check_finalize_();
  };

  if (!m_delta.override_x.has_value() && (no_axis_constraints || m_opts.constr_axis_x))
    gui().set_dist_edit(float(m_delta.delta.X() / view().get_dimension_scale()), std::move(m_dist_axis_x));

  else if (!m_delta.override_y.has_value() && (no_axis_constraints || m_opts.constr_axis_y))
    gui().set_dist_edit(float(m_delta.delta.Y() / view().get_dimension_scale()), std::move(m_dist_axis_y));

  else if (!m_delta.override_z.has_value() && (no_axis_constraints || m_opts.constr_axis_z))
    gui().set_dist_edit(float(m_delta.delta.Z() / view().get_dimension_scale()), std::move(m_dist_axis_z));
}

bool Shp_move::check_finalize_()
{
  bool no_axis_constraints = !m_opts.constr_axis_x && !m_opts.constr_axis_y && !m_opts.constr_axis_z;
  if (no_axis_constraints)
  {
    if (m_delta.override_x.has_value() && m_delta.override_y.has_value() && m_delta.override_z.has_value())
    {
      finalize_move_selected();
      return true;
    }
  }
  else
  {
    bool got_x = !m_opts.constr_axis_x || m_delta.override_x.has_value();
    bool got_y = !m_opts.constr_axis_y || m_delta.override_y.has_value();
    bool got_z = !m_opts.constr_axis_z || m_delta.override_z.has_value();

    if (got_x && got_y && got_z)
    {
      finalize_move_selected();
      return true;
    }
  }

  return false;
}

void Shp_move::finalize_move_selected()
{
  std::vector<AIS_Shape_ptr> selected = get_selected();
  if (selected.empty())
    return;

  for (AIS_Shape_ptr& shape : selected)
    view().bake_transform_into_geometry(shape);

  post_opts_();
}

void Shp_move::cancel_move_selected()
{
  for (AIS_Shape_ptr& shape : get_selected())
    shape->ResetTransformation();

  post_opts_();
}

void Shp_move::post_opts_()
{
  // Reset options
  m_opts  = {};
  m_delta = {};
  gui().set_mode(Mode::Normal);
}

Move_options& Shp_move::get_opts()
{
  return m_opts;
}