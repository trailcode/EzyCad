#include "shapes.h"

#include <AIS_InteractiveContext.hxx>

Shape_base::Shape_base(AIS_InteractiveContext& ctx,
                     const TopoDS_Shape&     shp,
                     const std::string&      name,
                     const Shape_type         type)
    : AIS_Shape(shp),
      m_ctx(ctx),
      m_name(name),
      m_type(type),
      m_disp_mode(AIS_Shaded),
      m_visible(true),
      m_selection_mode(TopAbs_SHAPE) {}

Shape_base::~Shape_base() {}

const std::string& Shape_base::get_name() const
{
  return m_name;
}
void Shape_base::set_name(const std::string& name)
{
  m_name = name;
}

Shape_type Shape_base::get_type() const
{
  return m_type;
}

Shape_type Shape_base::get_type(const std::string& shp_type_name)
{
  for (size_t i = 0; i < c_ShapeType_names.size(); ++i)
    if (c_ShapeType_names[i] == shp_type_name)
      return Shape_type(i);

  DO_ASSERT_MSG(false, "Unknown shape type: " + shp_type_name);
  return Shape_type::_count;
}

const std::string_view& Shape_base::get_type_str() const
{
  return c_ShapeType_names[size_t(m_type)];
}

AIS_DisplayMode Shape_base::get_disp_mode() const
{
  return m_disp_mode;
}

void Shape_base::set_disp_mode(const AIS_DisplayMode mode)
{
  m_disp_mode = mode;
  update_display_();
}

bool Shape_base::get_visible() const
{
  return m_visible;
}

void Shape_base::set_visible(const bool visible)
{
  m_visible = visible;
  if (visible)
  {
    m_ctx.Activate(this, AIS_Shape::SelectionMode(m_selection_mode));
    m_ctx.Display(this, m_disp_mode, AIS_Shape::SelectionMode(m_selection_mode), true);
  }
  else
    m_ctx.Erase(this, true);
}

void Shape_base::set_selection_mode(const TopAbs_ShapeEnum mode)
{
  m_selection_mode = mode;
  update_display_();
}

void Shape_base::update_display_()
{
  if (!get_visible())
    return;

  m_ctx.Activate(this, AIS_Shape::SelectionMode(m_selection_mode));
  m_ctx.Display(this, m_disp_mode, AIS_Shape::SelectionMode(m_selection_mode), true);
}

ExtrudedShp::ExtrudedShp(AIS_InteractiveContext& ctx, const TopoDS_Shape& shp)
    : Shape_base(ctx, shp, "Extruded", Shape_type::Extruded) {}

RevolvedShp::RevolvedShp(AIS_InteractiveContext& ctx, const TopoDS_Shape& shp)
    : Shape_base(ctx, shp, "Revolved", Shape_type::Revolved) {}

ChamferShp::ChamferShp(AIS_InteractiveContext& ctx, const TopoDS_Shape& shp)
    : Shape_base(ctx, shp, "Chamfer", Shape_type::Chamfer) {}