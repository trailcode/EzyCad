#include "shp.h"

#include <AIS_InteractiveContext.hxx>

Shp::Shp(AIS_InteractiveContext& ctx, const TopoDS_Shape& shp)
    : AIS_Shape(shp)
    , m_ctx(ctx)
    , m_id(0)
    , m_name("Shape")
    , m_disp_mode(AIS_Shaded)
    , m_visible(true)
    , m_selection_mode(TopAbs_SHAPE)
{
}

Shp::~Shp() {}

Shape_id Shp::get_id() const { return m_id; }

void Shp::set_id(Shape_id id) { m_id = id; }

const std::string& Shp::get_name() const { return m_name; }

void Shp::set_name(const std::string& name) { m_name = name; }

AIS_DisplayMode Shp::get_disp_mode() const { return m_disp_mode; }

void Shp::set_disp_mode(const AIS_DisplayMode mode)
{
  m_disp_mode = mode;
  update_display_();
}

bool Shp::get_visible() const { return m_visible; }

void Shp::set_visible(const bool visible)
{
  if (m_visible == visible)
    return;

  m_visible = visible;
  if (visible)
  {
    redisplay_();
  }
  else
  {
    m_ctx.Unhilight(this, false);
    m_ctx.Erase(this, true);
  }
}

void Shp::set_selection_mode(const TopAbs_ShapeEnum mode)
{
  m_selection_mode = mode;
  update_display_();
}

void Shp::set_sketch_faint(bool enabled, AIS_DisplayMode faint_mode, float transparency)
{
  m_sketch_faint_active = enabled;
  m_faint_disp_mode     = faint_mode;
  SetTransparency(enabled ? static_cast<double>(transparency) : 0.0);
  update_display_();
}

AIS_DisplayMode Shp::effective_disp_mode_() const
{
  return m_sketch_faint_active ? m_faint_disp_mode : m_disp_mode;
}

void Shp::redisplay_()
{
  m_ctx.Unhilight(this, false);
  // Faint sketch-mode shapes are display-only (no pick/hover highlight).
  if (m_sketch_faint_active)
  {
    m_ctx.Display(this, effective_disp_mode_(), -1, false);
    m_ctx.Deactivate(this);
  }
  else
  {
    m_ctx.Activate(this, AIS_Shape::SelectionMode(m_selection_mode));
    m_ctx.Display(this, effective_disp_mode_(), AIS_Shape::SelectionMode(m_selection_mode), false);
  }
}

void Shp::update_display_()
{
  if (!get_visible())
    return;

  // Required to update selection mode in some cases.
  // E.G. after changing from create sketch from face to normal mode.
  m_ctx.Erase(this, false);
  redisplay_();
  m_ctx.UpdateCurrentViewer();
}
