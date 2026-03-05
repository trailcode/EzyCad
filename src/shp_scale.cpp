#include "shp_scale.h"

#include "geom.h"
#include "gui.h"
#include "modes.h"
#include "occt_view.h"
#include "utl.h"

Shp_scale::Shp_scale(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_scale::scale_selected(const ScreenCoords& screen_coords)
{
  CHK_RET(ensure_start_state_());

  std::optional<gp_Pnt> mouse_wc_pos = view().pt3d_on_plane(screen_coords, *m_scale_pln);
  if (!mouse_wc_pos)
    return Status::user_error("Adjust view, cannot get point on plane.");

  const double dist = m_center->Distance(*mouse_wc_pos);
  if (dist < Precision::Confusion())
    return Status::user_error("Move mouse further from scale center to start scaling.");

  if (m_initial_distance < Precision::Confusion())
    m_initial_distance = dist;

  m_scale_factor = dist / m_initial_distance;
  if (m_scale_factor < 0.01)
    m_scale_factor = 0.01;
  if (m_scale_factor > 100.0)
    m_scale_factor = 100.0;

  preview_scale_();

  return Status::ok();
}

Status Shp_scale::ensure_start_state_()
{
  CHK_RET(ensure_operation_shps_());

  if (!m_center.has_value())
    m_center = get_shape_bbox_center(m_shps[0]->Shape());

  if (!m_scale_pln.has_value())
    m_scale_pln = view().get_view_plane(*m_center);

  return Status::ok();
}

void Shp_scale::preview_scale_()
{
  EZY_ASSERT(m_center.has_value());

  gp_Trsf scale_trsf;
  scale_trsf.SetScale(*m_center, m_scale_factor);

  for (const AIS_Shape_ptr& shape : m_shps)
  {
    shape->SetLocalTransformation(scale_trsf);
    ctx().Redisplay(shape, true);
  }
}

void Shp_scale::finalize()
{
  if (m_shps.empty())
    return;

  operation_shps_finalize_();
  reset();
}

void Shp_scale::reset()
{
  if (!m_shps.empty())
    operation_shps_cancel_();

  m_shps.clear();
  m_scale_pln.reset();
  m_center.reset();
  m_initial_distance = 0;
  m_scale_factor     = 1.0;
}

void Shp_scale::cancel()
{
  operation_shps_cancel_();
  reset();
  gui().set_mode(Mode::Normal);
}
