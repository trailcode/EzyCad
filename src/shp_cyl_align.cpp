#include "shp_cyl_align.h"

#include <Precision.hxx>
#include <cmath>
#include <gp_Ax2.hxx>

#include "gui.h"
#include "gui_occt_view.h"
#include "mode.h"
#include "shp_delta.h"
#include "utl.h"
#include "utl_geom.h"

Shp_cyl_align::Shp_cyl_align(Occt_view& view)
    : Shp_operation_base(view)
{
}

void Shp_cyl_align::begin()
{
  // Keep m_opts (Flip / Clock rotation) across sessions, like Extrude Twist.
  clear_all(m_phase, m_axial_offset, m_moving_radius, m_fixed_radius, m_depth_override, m_moving_shp, m_fixed_shp,
            m_moving_axis, m_fixed_axis, m_drag_pln, m_twist_angle, m_twist_override, m_twist_angle0, m_twist_pln, m_shps);
}

bool Shp_cyl_align::is_dragging() const
{
  return (m_phase == Phase::Drag_depth || m_phase == Phase::Drag_twist) && !m_shps.empty();
}

bool Shp_cyl_align::is_twist_phase() const { return m_phase == Phase::Drag_twist && !m_shps.empty(); }

Cyl_align_options& Shp_cyl_align::get_opts() { return m_opts; }

void Shp_cyl_align::set_clock_rotation_enabled(bool enabled)
{
  if (m_opts.clock_rotation == enabled)
    return;

  m_opts.clock_rotation = enabled;
  if (!enabled && m_phase == Phase::Drag_twist)
    exit_twist_to_depth_();
}

void Shp_cyl_align::apply_preview()
{
  if (is_dragging())
    apply_preview_();
}

Status Shp_cyl_align::pick(const ScreenCoords& screen_coords)
{
  if (is_dragging())
    return Status::ok();

  Shp_ptr shp = Shp_ptr::DownCast(get_shape_(screen_coords));
  if (shp.IsNull() || shp->is_group())
    return Status::user_error("Click a cylindrical face on a solid.");

  const TopoDS_Face* face = get_face_(screen_coords);
  if (!face)
    return Status::user_error("Click a face (selection filter is Face).");

  const std::optional<Cyl_face_info> cyl = cylinder_from_face(*face);
  if (!cyl)
    return Status::user_error("Selected face is not cylindrical.");

  if (m_phase == Phase::Pick_moving)
  {
    m_moving_shp    = shp;
    m_moving_axis   = cyl->axis;
    m_moving_radius = cyl->radius;
    m_phase         = Phase::Pick_fixed;
    gui().show_message("Pick the fixed cylindrical face (hole or shaft).");
    return Status::ok();
  }

  // Pick_fixed
  if (shp == m_moving_shp)
    return Status::user_error("Pick a cylindrical face on a different shape.");

  m_fixed_shp    = shp;
  m_fixed_axis   = cyl->axis;
  m_fixed_radius = cyl->radius;

  if (std::abs(m_moving_radius - m_fixed_radius) > Precision::Confusion())
    gui().log_message("Align shafts: radii differ (moving " + std::to_string(m_moving_radius) + ", fixed " +
                      std::to_string(m_fixed_radius) + "); placement still allowed.");

  enter_drag_();
  if (m_opts.clock_rotation)
    gui().show_message("Drag along the axis for insert depth, then LMB or Shift+Tab for clock rotation (or Enter to finish).");
  else
    gui().show_message("Drag along the axis for insert depth, then LMB or Enter to confirm.");

  return Status::ok();
}

void Shp_cyl_align::enter_drag_()
{
  EZY_ASSERT(!m_moving_shp.IsNull());
  EZY_ASSERT(m_moving_axis.has_value() && m_fixed_axis.has_value());

  clear_all(m_axial_offset, m_depth_override, m_drag_pln, m_twist_angle, m_twist_override, m_twist_angle0, m_twist_pln);
  set_operation_shps_({m_moving_shp});
  m_phase = Phase::Drag_depth;
  view().set_dynamic_highlight_enabled(false);
  apply_preview_();
}

void Shp_cyl_align::enter_twist_()
{
  EZY_ASSERT(m_phase == Phase::Drag_depth);
  EZY_ASSERT(m_opts.clock_rotation);
  EZY_ASSERT(m_fixed_axis.has_value() && m_moving_axis.has_value());

  // Commit any typed depth override into the live offset, then clear overrides.
  if (m_depth_override.has_value())
    m_axial_offset = *m_depth_override;

  clear_all(m_depth_override, m_twist_angle, m_twist_override, m_twist_angle0, m_twist_pln);
  gui().hide_dist_edit(false);

  const gp_Dir& fixed_dir = m_fixed_axis->Direction();
  const gp_Vec  to_moving(m_fixed_axis->Location(), m_moving_axis->Location());
  const double  param0 = to_moving.Dot(gp_Vec(fixed_dir));
  const gp_Pnt  seed   = m_fixed_axis->Location().Translated(gp_Vec(fixed_dir) * (param0 + m_axial_offset));
  m_twist_pln          = gp_Pln(seed, fixed_dir);

  m_phase = Phase::Drag_twist;
  gui().show_message("Drag to clock about the axis, then LMB or Enter to confirm.");
  apply_preview_();
}

void Shp_cyl_align::exit_twist_to_depth_()
{
  EZY_ASSERT(m_phase == Phase::Drag_twist);
  clear_all(m_twist_angle, m_twist_override, m_twist_angle0, m_twist_pln);
  gui().hide_angle_edit(false);
  m_phase = Phase::Drag_depth;
  apply_preview_();
  gui().show_message("Drag along the axis for insert depth, then LMB or Enter to confirm.");
}

void Shp_cyl_align::apply_preview_()
{
  EZY_ASSERT(m_moving_axis.has_value() && m_fixed_axis.has_value());
  EZY_ASSERT(!m_shps.empty());

  const double  offset = m_depth_override.value_or(m_axial_offset);
  const double  twist  = m_twist_override.value_or(m_twist_angle);
  const gp_Trsf trsf   = cyl_align_trsf(*m_moving_axis, *m_fixed_axis, m_opts.flip_direction, offset, twist);

  for (const Shp_ptr& shape : m_shps)
    shape->SetLocalTransformation(trsf);

  redisplay_operation_shps_after_transform_();
}

Status Shp_cyl_align::drag_depth(const ScreenCoords& screen_coords)
{
  if (m_phase != Phase::Drag_depth)
    return Status::ok();

  EZY_ASSERT(m_fixed_axis.has_value() && m_moving_axis.has_value());

  if (m_depth_override.has_value())
  {
    apply_preview_();
    return Status::ok();
  }

  const gp_Dir& fixed_dir  = m_fixed_axis->Direction();
  const gp_Vec  to_moving  = {m_fixed_axis->Location(), m_moving_axis->Location()};
  const double  param0     = to_moving.Dot(gp_Vec(fixed_dir));
  const gp_Pnt  seed_on_ax = m_fixed_axis->Location().Translated(gp_Vec(fixed_dir) * param0);

  if (!m_drag_pln.has_value())
    m_drag_pln = view().get_view_plane(seed_on_ax);

  const std::optional<gp_Pnt> mouse_wc = view().pt3d_on_plane(screen_coords, *m_drag_pln);
  if (!mouse_wc)
    return Status::user_error("Adjust view, cannot get point on plane.");

  const gp_Vec from_seed(seed_on_ax, *mouse_wc);
  m_axial_offset = from_seed.Dot(gp_Vec(fixed_dir));
  apply_preview_();

  return Status::ok();
}

Status Shp_cyl_align::drag_twist(const ScreenCoords& screen_coords)
{
  if (m_phase != Phase::Drag_twist)
    return Status::ok();

  EZY_ASSERT(m_fixed_axis.has_value());
  EZY_ASSERT(m_twist_pln.has_value());

  if (m_twist_override.has_value())
  {
    apply_preview_();
    return Status::ok();
  }

  const std::optional<gp_Pnt> mouse_wc = view().pt3d_on_plane(screen_coords, *m_twist_pln);
  if (!mouse_wc)
    return Status::user_error("Adjust view, cannot get point on plane.");

  const gp_Ax2 ax2(m_twist_pln->Location(), m_fixed_axis->Direction());
  const gp_Vec v(m_twist_pln->Location(), *mouse_wc);
  const double vx = v.Dot(gp_Vec(ax2.XDirection()));
  const double vy = v.Dot(gp_Vec(ax2.YDirection()));
  if (std::hypot(vx, vy) <= Precision::Confusion())
    return Status::ok();

  const double ang = std::atan2(vy, vx);
  if (!m_twist_angle0.has_value())
    m_twist_angle0 = ang;

  m_twist_angle = ang - *m_twist_angle0;
  apply_preview_();

  return Status::ok();
}

void Shp_cyl_align::on_left_click()
{
  if (m_phase == Phase::Drag_depth)
  {
    if (m_opts.clock_rotation)
      enter_twist_();
    else
      finalize();
  }
  else if (m_phase == Phase::Drag_twist)
    finalize();
}

void Shp_cyl_align::begin_twist_input(const ScreenCoords& screen_coords)
{
  if (!m_opts.clock_rotation || !is_dragging())
    return;

  if (m_phase == Phase::Drag_depth)
    enter_twist_();

  if (m_phase == Phase::Drag_twist)
    show_twist_edit(screen_coords);
}

void Shp_cyl_align::show_depth_edit(const ScreenCoords& screen_coords)
{
  if (m_phase != Phase::Drag_depth)
    return;

  auto depth_edit = [&, screen_coords](float new_dist, bool is_final)
  {
    m_depth_override = new_dist * view().get_display_to_model_scale();
    EZY_ASSERT(drag_depth(screen_coords).is_ok());
    if (is_final)
    {
      if (m_opts.clock_rotation)
        enter_twist_();
      else
        finalize();
    }
  };

  const double cur = m_depth_override.value_or(m_axial_offset);
  gui().set_dist_edit(float(cur / view().get_display_to_model_scale()),
                      std::move(std::function<void(float, bool)>(depth_edit)));
}

void Shp_cyl_align::show_twist_edit(const ScreenCoords& screen_coords)
{
  if (m_phase != Phase::Drag_twist)
    return;

  auto twist_edit = [&, screen_coords](float new_angle_deg, bool is_final)
  {
    m_twist_override = to_radians(static_cast<double>(new_angle_deg));
    m_twist_angle    = *m_twist_override;
    EZY_ASSERT(drag_twist(screen_coords).is_ok());
    if (is_final)
      finalize();
  };

  const double cur = m_twist_override.value_or(m_twist_angle);
  gui().set_angle_edit(float(to_degrees(cur)), std::move(std::function<void(float, bool)>(twist_edit)), screen_coords);
}

void Shp_cyl_align::finalize()
{
  if (!is_dragging())
    return;

  // Commit typed overrides before bake.
  if (m_depth_override.has_value())
    m_axial_offset = *m_depth_override;

  if (m_twist_override.has_value())
    m_twist_angle = *m_twist_override;

  clear_all(m_depth_override, m_twist_override);
  apply_preview_();

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

void Shp_cyl_align::cancel()
{
  if (is_dragging())
    operation_shps_cancel_();

  reset();
  restore_operation_selection_();
}

void Shp_cyl_align::reset()
{
  begin();
  gui().set_mode(Mode::Normal);
}
