#include "shp_cyl_align.h"

#include <Precision.hxx>

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
  clear_all(m_phase, m_opts, m_axial_offset, m_moving_radius, m_fixed_radius, m_depth_override, m_moving_shp, m_fixed_shp,
            m_moving_axis, m_fixed_axis, m_drag_pln, m_shps);
}

bool Shp_cyl_align::is_dragging() const { return m_phase == Phase::Drag_depth && !m_shps.empty(); }

Cyl_align_options& Shp_cyl_align::get_opts() { return m_opts; }

void Shp_cyl_align::apply_preview()
{
  if (is_dragging())
    apply_preview_();
}

Status Shp_cyl_align::pick(const ScreenCoords& screen_coords)
{
  if (m_phase == Phase::Drag_depth)
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
    gui().log_message("Align cylinders: radii differ (moving " + std::to_string(m_moving_radius) + ", fixed " +
                      std::to_string(m_fixed_radius) + "); placement still allowed.");

  enter_drag_();
  gui().show_message("Drag along the axis for insert depth, then LMB or Enter to confirm.");
  return Status::ok();
}

void Shp_cyl_align::enter_drag_()
{
  EZY_ASSERT(!m_moving_shp.IsNull());
  EZY_ASSERT(m_moving_axis.has_value() && m_fixed_axis.has_value());

  clear_all(m_axial_offset, m_depth_override, m_drag_pln);
  set_operation_shps_({m_moving_shp});
  m_phase = Phase::Drag_depth;
  view().set_dynamic_highlight_enabled(false);
  apply_preview_();
}

void Shp_cyl_align::apply_preview_()
{
  EZY_ASSERT(m_moving_axis.has_value() && m_fixed_axis.has_value());
  EZY_ASSERT(!m_shps.empty());

  const double  offset = m_depth_override.value_or(m_axial_offset);
  const gp_Trsf trsf   = cyl_align_trsf(*m_moving_axis, *m_fixed_axis, m_opts.flip_direction, offset);

  for (const Shp_ptr& shape : m_shps)
    shape->SetLocalTransformation(trsf);

  redisplay_operation_shps_after_transform_();
}

Status Shp_cyl_align::drag_depth(const ScreenCoords& screen_coords)
{
  if (!is_dragging())
    return Status::ok();

  EZY_ASSERT(m_fixed_axis.has_value() && m_moving_axis.has_value());

  if (m_depth_override.has_value())
  {
    apply_preview_();
    return Status::ok();
  }

  const gp_Dir& fixed_dir = m_fixed_axis->Direction();
  const gp_Vec  to_moving(m_fixed_axis->Location(), m_moving_axis->Location());
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

void Shp_cyl_align::show_depth_edit(const ScreenCoords& screen_coords)
{
  if (!is_dragging())
    return;

  auto depth_edit = [&, screen_coords](float new_dist, bool is_final)
  {
    m_depth_override = new_dist * view().get_display_to_model_scale();
    EZY_ASSERT(drag_depth(screen_coords).is_ok());
    if (is_final)
      finalize();
  };

  const double cur = m_depth_override.value_or(m_axial_offset);
  gui().set_dist_edit(float(cur / view().get_display_to_model_scale()),
                      std::move(std::function<void(float, bool)>(depth_edit)));
}

void Shp_cyl_align::finalize()
{
  if (!is_dragging())
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
