#include "shp.h"

#include <AIS_InteractiveContext.hxx>

Shp::Shp(AIS_InteractiveContext& ctx, const TopoDS_Shape& shp)
    : AIS_Shape(shp),
      m_ctx(ctx),
      m_name("Shape"),
      m_disp_mode(AIS_Shaded),
      m_visible(true),
      m_selection_mode(TopAbs_SHAPE) {
}

Shp::~Shp() {}

const std::string& Shp::get_name() const {
  return m_name;
}

void Shp::set_name(const std::string& name) {
  m_name = name;
}

AIS_DisplayMode Shp::get_disp_mode() const {
  return m_disp_mode;
}

void Shp::set_disp_mode(const AIS_DisplayMode mode) {
  m_disp_mode = mode;
  update_display_();
}

bool Shp::get_visible() const {
  return m_visible;
}

void Shp::set_visible(const bool visible) {
  if (m_visible == visible)
    return;

  m_visible = visible;
  if (visible) {
    m_ctx.Activate(this, AIS_Shape::SelectionMode(m_selection_mode));
    m_ctx.Display(this, m_disp_mode, AIS_Shape::SelectionMode(m_selection_mode), true);
  } else
    m_ctx.Erase(this, true);
}

void Shp::set_selection_mode(const TopAbs_ShapeEnum mode) {
  m_selection_mode = mode;
  update_display_();
}

void Shp::update_display_() {
  if (!get_visible())
    return;

  // Required to update selection mode in some cases.
  // E.G. after changing from create sketch from face to normal mode.
  m_ctx.Erase(this, true);
  m_ctx.Activate(this, AIS_Shape::SelectionMode(m_selection_mode));
  m_ctx.Display(this, m_disp_mode, AIS_Shape::SelectionMode(m_selection_mode), true);
}
