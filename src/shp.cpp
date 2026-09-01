#include "shp.h"

#include <AIS_InteractiveContext.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <Quantity_Color.hxx>
#include <TopoDS_Compound.hxx>
#include <cmath>

namespace
{
gp_Ax3 default_shape_frame_(const TopoDS_Shape& shape);
double frame_arm_length_(const TopoDS_Shape& shape);
} // namespace

Shp::Shp(AIS_InteractiveContext& ctx, const TopoDS_Shape& shp)
    : AIS_Shape(shp)
    , m_ctx(ctx)
    , m_id(0)
    , m_name("Shape")
    , m_disp_mode(AIS_Shaded)
    , m_visible(true)
    , m_selection_mode(TopAbs_SHAPE)
    , m_frame(default_shape_frame_(shp))
{
}

Shp::~Shp()
{
  // Do not call m_ctx.Remove here: on app exit AIS may release Shp after the context
  // is already dying (shapes stay alive via Display handles). Nullify only.
  m_frame_axis_x_ais.Nullify();
  m_frame_axis_y_ais.Nullify();
  m_frame_axis_z_ais.Nullify();
  m_frame_plane_fill_ais.Nullify();
  m_frame_plane_lines_ais.Nullify();
  m_frame_up_ais.Nullify();
}

Shp_ptr Shp::create_group(AIS_InteractiveContext& ctx, const std::string& name)
{
  TopoDS_Compound comp;
  BRep_Builder().MakeCompound(comp);
  Shp_ptr grp     = new Shp(ctx, comp);
  grp->m_is_group = true;
  grp->set_name(name);
  grp->m_visible = true;
  return grp;
}

Shape_id Shp::get_id() const { return m_id; }

void Shp::set_id(Shape_id id) { m_id = id; }

const std::string& Shp::get_name() const { return m_name; }

void Shp::set_name(const std::string& name) { m_name = name; }

AIS_DisplayMode Shp::get_disp_mode() const { return m_disp_mode; }

void Shp::set_disp_mode(const AIS_DisplayMode mode)
{
  m_disp_mode = mode;
  if (!m_is_group)
    update_display_();
}

bool Shp::get_visible() const { return m_visible; }

void Shp::set_visible(const bool visible)
{
  if (m_visible == visible)
    return;

  m_visible = visible;
  if (m_is_group)
    return;

  if (visible)
    redisplay_();
  else
  {
    m_ctx.Unhilight(this, false);
    m_ctx.Erase(this, true);
  }

  update_frame_display();
}

void Shp::apply_context_shown(bool shown)
{
  if (m_is_group)
    return;

  if (shown)
    redisplay_();
  else
  {
    m_ctx.Unhilight(this, false);
    m_ctx.Erase(this, false);
  }

  // Overlay hide (Hide all, hidden ancestor, sketch-hide) must drop frame AIS even when
  // get_visible() is still true. update_frame_display() keys off m_visible + suppressed.
  if (shown)
    update_frame_display();
  else
    clear_frame_display();
}

void Shp::set_selection_mode(const TopAbs_ShapeEnum mode)
{
  m_selection_mode = mode;
  if (!m_is_group)
    update_display_();
}

void Shp::set_sketch_faint(bool enabled, AIS_DisplayMode faint_mode, float transparency)
{
  if (m_is_group)
    return;

  m_sketch_faint_active = enabled;
  m_faint_disp_mode     = faint_mode;
  SetTransparency(enabled ? static_cast<double>(transparency) : 0.0);
  update_display_();
}

AIS_DisplayMode Shp::effective_disp_mode_() const { return m_sketch_faint_active ? m_faint_disp_mode : m_disp_mode; }

void Shp::redisplay_()
{
  if (m_is_group)
    return;

  m_ctx.Unhilight(this, false);
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
  if (m_is_group || !get_visible())
    return;

  m_ctx.Erase(this, false);
  redisplay_();
  m_ctx.UpdateCurrentViewer();
}

void Shp::set_frame(const gp_Ax3& frame)
{
  m_frame = frame;
  update_frame_display();
}

void Shp::transform_frame(const gp_Trsf& transform)
{
  m_frame.Transform(transform);
  update_frame_display();
}

gp_Ax3 Shp::default_frame_for(const TopoDS_Shape& shape) { return default_shape_frame_(shape); }

void Shp::set_show_frame_axes(bool show)
{
  if (m_show_frame_axes == show)
    return;
  m_show_frame_axes = show;
  update_frame_display();
}

void Shp::set_show_frame_plane(bool show)
{
  if (m_show_frame_plane == show)
    return;
  m_show_frame_plane = show;
  update_frame_display();
}

void Shp::set_show_frame_up(bool show)
{
  if (m_show_frame_up == show)
    return;
  m_show_frame_up = show;
  update_frame_display();
}

void Shp::set_frame_display_suppressed(bool suppressed)
{
  if (m_frame_display_suppressed == suppressed)
    return;
  m_frame_display_suppressed = suppressed;
  update_frame_display();
}

void Shp::clear_frame_display()
{
  auto remove = [&](AIS_Shape_ptr& ais)
  {
    if (!ais.IsNull())
      m_ctx.Remove(ais, false);

    ais.Nullify();
  };

  remove(m_frame_axis_x_ais);
  remove(m_frame_axis_y_ais);
  remove(m_frame_axis_z_ais);
  remove(m_frame_plane_fill_ais);
  remove(m_frame_plane_lines_ais);
  remove(m_frame_up_ais);
}

void Shp::sync_frame_display_trsf()
{
  const gp_Trsf& trsf  = LocalTransformation();
  auto           apply = [&](const AIS_Shape_ptr& ais)
  {
    if (!ais.IsNull())
      ais->SetLocalTransformation(trsf);
  };

  apply(m_frame_axis_x_ais);
  apply(m_frame_axis_y_ais);
  apply(m_frame_axis_z_ais);
  apply(m_frame_plane_fill_ais);
  apply(m_frame_plane_lines_ais);
  apply(m_frame_up_ais);
}

void Shp::update_frame_display()
{
  clear_frame_display();
  if (m_is_group || !m_visible || m_frame_display_suppressed)
    return;

  if (!m_show_frame_axes && !m_show_frame_plane && !m_show_frame_up)
    return;

  const double arm = frame_arm_length_(Shape());
  if (arm <= 0.0)
    return;

  const gp_Pnt  o = m_frame.Location();
  const gp_Vec  x(m_frame.XDirection());
  const gp_Vec  y(m_frame.YDirection());
  const gp_Vec  z(m_frame.Direction());
  const gp_Trsf trsf = LocalTransformation();

  auto display_wire = [&](const TopoDS_Shape& geom, Quantity_NameOfColor color, double width) -> AIS_Shape_ptr
  {
    AIS_Shape_ptr ais = new AIS_Shape(geom);
    ais->SetColor(color);
    ais->SetWidth(width);
    ais->SetZLayer(Graphic3d_ZLayerId_Topmost);
    ais->SetLocalTransformation(trsf);
    m_ctx.Display(ais, AIS_WireFrame, -1, false);
    m_ctx.Deactivate(ais);
    return ais;
  };

  if (m_show_frame_axes)
  {
    // Standard CAD triad: X red, Y green, Z blue.
    m_frame_axis_x_ais = display_wire(BRepBuilderAPI_MakeEdge(o, o.Translated(x * arm)).Edge(), Quantity_NOC_RED, 2.0);
    m_frame_axis_y_ais = display_wire(BRepBuilderAPI_MakeEdge(o, o.Translated(y * arm)).Edge(), Quantity_NOC_GREEN, 2.0);
    m_frame_axis_z_ais =
        display_wire(BRepBuilderAPI_MakeEdge(o, o.Translated(z * (arm * 1.15))).Edge(), Quantity_NOC_BLUE1, 2.5);
  }

  if (m_show_frame_plane)
  {
    const double      half = arm * 0.75;
    const gp_Pln      pln(m_frame);
    const TopoDS_Face face = BRepBuilderAPI_MakeFace(pln, -half, half, -half, half).Face();
    m_frame_plane_fill_ais = new AIS_Shape(face);
    m_frame_plane_fill_ais->SetColor(Quantity_NOC_CYAN);
    m_frame_plane_fill_ais->SetTransparency(0.85);
    m_frame_plane_fill_ais->SetLocalTransformation(trsf);
    m_ctx.Display(m_frame_plane_fill_ais, AIS_Shaded, -1, false);
    m_ctx.Deactivate(m_frame_plane_fill_ais);

    BRepBuilderAPI_MakePolygon outline;
    outline.Add(o.Translated(x * (-half) + y * (-half)));
    outline.Add(o.Translated(x * half + y * (-half)));
    outline.Add(o.Translated(x * half + y * half));
    outline.Add(o.Translated(x * (-half) + y * half));
    outline.Close();
    m_frame_plane_lines_ais = display_wire(outline.Wire(), Quantity_NOC_CYAN, 1.5);
  }

  if (m_show_frame_up)
  {
    const gp_Pnt    tip         = o.Translated(y * arm);
    const gp_Pnt    arrow_left  = tip.Translated(y * (-arm * 0.2) + x * (arm * 0.1));
    const gp_Pnt    arrow_right = tip.Translated(y * (-arm * 0.2) - x * (arm * 0.1));
    TopoDS_Compound up;
    BRep_Builder    builder;
    builder.MakeCompound(up);
    builder.Add(up, BRepBuilderAPI_MakeEdge(o, tip).Edge());
    builder.Add(up, BRepBuilderAPI_MakeEdge(arrow_left, tip).Edge());
    builder.Add(up, BRepBuilderAPI_MakeEdge(arrow_right, tip).Edge());
    m_frame_up_ais = display_wire(up, Quantity_NOC_GREEN, 3.0);
  }

  m_ctx.UpdateCurrentViewer();
}

namespace
{
gp_Ax3 default_shape_frame_(const TopoDS_Shape& shape)
{
  if (shape.IsNull())
    return gp_Ax3();

  Bnd_Box bounds;
  BRepBndLib::Add(shape, bounds);
  if (bounds.IsVoid())
    return gp_Ax3();

  double x_min, y_min, z_min, x_max, y_max, z_max;
  bounds.Get(x_min, y_min, z_min, x_max, y_max, z_max);
  return gp_Ax3(gp_Pnt((x_min + x_max) * 0.5, (y_min + y_max) * 0.5, (z_min + z_max) * 0.5), gp::DZ(), gp::DX());
}

double frame_arm_length_(const TopoDS_Shape& shape)
{
  if (shape.IsNull())
    return 1.0;

  Bnd_Box bounds;
  BRepBndLib::Add(shape, bounds);
  if (bounds.IsVoid())
    return 1.0;

  double x_min, y_min, z_min, x_max, y_max, z_max;
  bounds.Get(x_min, y_min, z_min, x_max, y_max, z_max);
  const double dx   = x_max - x_min;
  const double dy   = y_max - y_min;
  const double dz   = z_max - z_min;
  const double diag = std::sqrt(dx * dx + dy * dy + dz * dz);
  return std::max(diag * 0.35, 1e-3);
}
} // namespace
