#include "shp_move.h"
#include "geom.h"
#include "gui.h"
#include "occt_view.h"

Shp_move::Shp_move(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_move::move_selected(const ScreenCoords& screen_coords)
{
  CHK_RET(ensure_operation_shps_());

  if (!m_center.has_value())
    // Get the estimate of the center.
    // TODO consider all shapes.
    m_center = get_shape_bbox_center(m_shps[0]->Shape());

  if (!m_move_pln.has_value())
    // Remember, if the user can change the view via a hot key this will be invalid.
    m_move_pln = view().get_view_plane(*m_center);

  std::optional<gp_Pnt> mouse_wc_pos = view().pt3d_on_plane(screen_coords, *m_move_pln);
  if (!mouse_wc_pos)
    return Status::user_error("Adjust view, cannot get point on plane.");
  
  bool no_axis_constraints = !m_opts.constr_axis_x && !m_opts.constr_axis_y && !m_opts.constr_axis_z;

  if (m_delta.override_x.has_value())
    m_delta.delta.SetX(*m_delta.override_x);
  else
    m_delta.delta.SetX(no_axis_constraints || m_opts.constr_axis_x ? mouse_wc_pos->X() - m_center->X() : 0);

  if (m_delta.override_y.has_value())
    m_delta.delta.SetY(*m_delta.override_y);
  else
    m_delta.delta.SetY(no_axis_constraints || m_opts.constr_axis_y ? mouse_wc_pos->Y() - m_center->Y() : 0);

  if (m_delta.override_z.has_value())
    m_delta.delta.SetZ(*m_delta.override_z);
  else
    m_delta.delta.SetZ(no_axis_constraints || m_opts.constr_axis_z ? mouse_wc_pos->Z() - m_center->Z() : 0);

  gp_Trsf translation;
  translation.SetTranslation(gp_Vec(m_delta.delta));

  for (const AIS_Shape_ptr& shape : m_shps)
  {
    shape->SetLocalTransformation(translation);
    ctx().Redisplay(shape, true);
  }

  return Status::ok();
}

void Shp_move::show_dist_edit(const ScreenCoords& screen_coords)
{
  bool no_axis_constraints = !m_opts.constr_axis_x && !m_opts.constr_axis_y && !m_opts.constr_axis_z;

  auto dist_edit_axis_x = [&, screen_coords](float new_dist, bool is_final)
  {
    m_delta.override_x = new_dist * view().get_dimension_scale();
    EZY_ASSERT(move_selected(screen_coords).is_ok()); // Status should always be valid here
    if (is_final)
      check_finalize_();
  };

  auto dist_edit_axis_y = [&, screen_coords](float new_dist, bool is_final)
  {
    m_delta.override_y = new_dist * view().get_dimension_scale();
    EZY_ASSERT(move_selected(screen_coords).is_ok());
    if (is_final)
      check_finalize_();
  };

  auto dist_edit_axis_z = [&, screen_coords](float new_dist, bool is_final)
  {
    m_delta.override_z = new_dist * view().get_dimension_scale();
    EZY_ASSERT(move_selected(screen_coords).is_ok());
    if (is_final)
      check_finalize_();
  };

  if (!m_delta.override_x.has_value() && (no_axis_constraints || m_opts.constr_axis_x))
    gui().set_dist_edit(float(m_delta.delta.X() / view().get_dimension_scale()), std::move(std::function<void(float, bool)>(dist_edit_axis_x)));

  else if (!m_delta.override_y.has_value() && (no_axis_constraints || m_opts.constr_axis_y))
    gui().set_dist_edit(float(m_delta.delta.Y() / view().get_dimension_scale()), std::move(std::function<void(float, bool)>(dist_edit_axis_y)));

  else if (!m_delta.override_z.has_value() && (no_axis_constraints || m_opts.constr_axis_z))
    gui().set_dist_edit(float(m_delta.delta.Z() / view().get_dimension_scale()), std::move(std::function<void(float, bool)>(dist_edit_axis_z)));
}

void Shp_move::check_finalize_()
{
  bool no_axis_constraints = !m_opts.constr_axis_x && !m_opts.constr_axis_y && !m_opts.constr_axis_z;
  if (no_axis_constraints)
  {
    if (m_delta.override_x.has_value() && m_delta.override_y.has_value() && m_delta.override_z.has_value())
      finalize();
  }
  else
  {
    bool got_x = !m_opts.constr_axis_x || m_delta.override_x.has_value();
    bool got_y = !m_opts.constr_axis_y || m_delta.override_y.has_value();
    bool got_z = !m_opts.constr_axis_z || m_delta.override_z.has_value();

    if (got_x && got_y && got_z)
      finalize();
  }
}

void Shp_move::finalize()
{
  if (m_shps.empty())
    // If the move tool is activated and no shapes are selected,
    // then we do not want to call post_opts_() because they could
    // be selected while in move mode.
    return;

  operation_shps_finalize_();
  post_opts_();
}

void Shp_move::cancel()
{
  operation_shps_cancel_();
  post_opts_();
}

void Shp_move::post_opts_()
{
  // Reset options
  m_opts  = {};
  m_delta = {};
  clear_all(m_move_pln, m_center, m_shps);
  gui().set_mode(Mode::Normal);
}

Move_options& Shp_move::get_opts()
{
  return m_opts;
}