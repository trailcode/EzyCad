#include "shp_section.h"

#include "gui_occt_view.h"
#include "utl_occt.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <GeomAbs_CurveType.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <Quantity_Color.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <sstream>
#include <vector>

namespace
{
gp_Pln                section_plane_(const gp_Ax3& frame, Section_plane plane, double offset);
void                  count_curve_type_(const TopoDS_Edge& edge, Section_geometry& result);
bool                  contains_solid_(const TopoDS_Shape& shape);
TopoDS_Shape          shape_world_(const Shp& shp);
gp_Ax3                frame_world_(const Shp& shp);
bool                  append_bounds_(const TopoDS_Shape& shape, Bnd_Box& bounds);
std::array<gp_Pnt, 8> bbox_corners_(const Bnd_Box& bounds);
void project_bbox_offsets_(const Bnd_Box& bounds, const gp_Pnt& origin, const gp_Dir& normal, double& out_min, double& out_max);
void add_plane_annotation_(const Bnd_Box& bounds, const gp_Pln& plane, BRep_Builder& builder, TopoDS_Compound& fill,
                           TopoDS_Compound& lines);
Result<Section_geometry> section_shape_on_plane_(const TopoDS_Shape& shape, const gp_Pln& plane);
} // namespace

Result<Section_geometry> section_shape(const TopoDS_Shape& shape, const gp_Ax3& frame, Section_plane plane, double offset)
{
  return section_shape_on_plane_(shape, section_plane_(frame, plane, offset));
}

Shp_section::Shp_section(Occt_view& view)
    : Shp_operation_base(view)
{
}

Status Shp_section::preview_selected()
{
  clear();
  if (!std::isfinite(m_offset_display))
    return Status::user_error("Section offset must be a finite number.");

  const std::vector<Shp_ptr> selected = get_selected_shps_();
  if (selected.empty())
    return Status::user_error("Select one or more solid shapes.");

  std::vector<TopoDS_Shape> world_shapes;
  world_shapes.reserve(selected.size());
  Bnd_Box combined_bounds;
  gp_Ax3  shared_axes;
  bool    have_axes = false;

  for (const Shp_ptr& shp : selected)
  {
    TopoDS_Shape world_shape = shape_world_(*shp);
    if (!contains_solid_(world_shape))
      return Status::user_error(shp->get_name() + ": Cross-section supports solid shapes only.");

    if (!append_bounds_(world_shape, combined_bounds))
      return Status::user_error(shp->get_name() + ": Shape has empty bounds.");

    if (!have_axes)
    {
      shared_axes = frame_world_(*shp);
      have_axes   = true;
    }

    world_shapes.push_back(std::move(world_shape));
  }

  if (combined_bounds.IsVoid() || !have_axes)
    return Status::user_error("Select one or more solid shapes.");

  double x_min, y_min, z_min, x_max, y_max, z_max;
  combined_bounds.Get(x_min, y_min, z_min, x_max, y_max, z_max);
  shared_axes.SetLocation(gp_Pnt((x_min + x_max) * 0.5, (y_min + y_max) * 0.5, (z_min + z_max) * 0.5));

  const double offset_model = view().to_model(m_offset_display);
  const gp_Pln shared_plane = section_plane_(shared_axes, m_plane, offset_model);

  TopoDS_Compound compound;
  TopoDS_Compound plane_fill;
  TopoDS_Compound plane_lines;
  BRep_Builder    builder;
  builder.MakeCompound(compound);
  builder.MakeCompound(plane_fill);
  builder.MakeCompound(plane_lines);

  Section_geometry totals;
  size_t           missed = 0;
  for (size_t i = 0; i < selected.size(); ++i)
  {
    Result<Section_geometry> result = section_shape_on_plane_(world_shapes[i], shared_plane);
    if (!result.has_value())
    {
      // A miss on one solid should not kill the shared preview for the others.
      if (result.status() == Result_status::User_error)
      {
        ++missed;
        continue;
      }

      return Status(result.status(), selected[i]->get_name() + ": " + result.message());
    }

    const Section_geometry& geometry = *result;
    builder.Add(compound, geometry.shape);
    totals.edge_count += geometry.edge_count;
    totals.line_count += geometry.line_count;
    totals.circle_count += geometry.circle_count;
    totals.ellipse_count += geometry.ellipse_count;
    totals.bspline_count += geometry.bspline_count;
    totals.other_curve_count += geometry.other_curve_count;
  }

  if (totals.edge_count == 0)
    return Status::user_error("The section plane does not intersect the selection.");

  add_plane_annotation_(combined_bounds, shared_plane, builder, plane_fill, plane_lines);

  m_preview = new AIS_Shape(compound);
  m_preview->SetColor(Quantity_NOC_CYAN);
  m_preview->SetWidth(3.0);
  m_preview->SetZLayer(Graphic3d_ZLayerId_Topmost);
  ctx().Display(m_preview, AIS_WireFrame, -1, false);
  ctx().Deactivate(m_preview);

  m_plane_fill = new AIS_Shape(plane_fill);
  m_plane_fill->SetColor(Quantity_NOC_YELLOW);
  m_plane_fill->SetTransparency(0.85);
  ctx().Display(m_plane_fill, AIS_Shaded, -1, false);
  ctx().Deactivate(m_plane_fill);

  m_plane_lines = new AIS_Shape(plane_lines);
  m_plane_lines->SetColor(Quantity_NOC_YELLOW);
  m_plane_lines->SetWidth(1.5);
  m_plane_lines->SetZLayer(Graphic3d_ZLayerId_Topmost);
  ctx().Display(m_plane_lines, AIS_WireFrame, -1, false);
  ctx().Deactivate(m_plane_lines);

  ctx().UpdateCurrentViewer();

  std::ostringstream msg;
  msg << "Section preview: " << totals.edge_count << " edge";
  if (totals.edge_count != 1)
    msg << "s";
  msg << " (" << totals.line_count << " line, " << totals.circle_count << " circle, " << totals.ellipse_count << " ellipse, "
      << totals.bspline_count << " B-spline, " << totals.other_curve_count << " other)";
  if (missed > 0)
  {
    msg << "; missed " << missed << " solid";
    if (missed != 1)
      msg << "s";
  }
  msg << ".";
  return Status::ok(msg.str());
}

void Shp_section::clear()
{
  if (!m_preview.IsNull())
    ctx().Remove(m_preview, false);

  if (!m_plane_fill.IsNull())
    ctx().Remove(m_plane_fill, false);

  if (!m_plane_lines.IsNull())
    ctx().Remove(m_plane_lines, false);

  m_preview.Nullify();
  m_plane_fill.Nullify();
  m_plane_lines.Nullify();
  ctx().UpdateCurrentViewer();
}

bool Shp_section::try_get_offset_range_display(double& out_min, double& out_max)
{
  const std::vector<Shp_ptr> selected = get_selected_shps_();
  if (selected.empty())
    return false;

  Bnd_Box combined_bounds;
  gp_Ax3  shared_axes;
  bool    have_axes = false;

  for (const Shp_ptr& shp : selected)
  {
    TopoDS_Shape world_shape = shape_world_(*shp);
    if (!contains_solid_(world_shape) || !append_bounds_(world_shape, combined_bounds))
      continue;

    if (!have_axes)
    {
      shared_axes = frame_world_(*shp);
      have_axes   = true;
    }
  }

  if (!have_axes || combined_bounds.IsVoid())
    return false;

  double x_min, y_min, z_min, x_max, y_max, z_max;
  combined_bounds.Get(x_min, y_min, z_min, x_max, y_max, z_max);
  shared_axes.SetLocation(gp_Pnt((x_min + x_max) * 0.5, (y_min + y_max) * 0.5, (z_min + z_max) * 0.5));

  const gp_Pln plane = section_plane_(shared_axes, m_plane, 0.0);
  double       model_min;
  double       model_max;
  project_bbox_offsets_(combined_bounds, plane.Location(), plane.Axis().Direction(), model_min, model_max);
  if (!(model_max > model_min))
    return false;

  out_min = view().to_display(model_min);
  out_max = view().to_display(model_max);
  if (out_min > out_max)
    std::swap(out_min, out_max);

  return true;
}

namespace
{
gp_Pln section_plane_(const gp_Ax3& frame, Section_plane plane, double offset)
{
  gp_Dir normal;
  gp_Dir x_dir;
  switch (plane)
  {
  case Section_plane::XY:
    normal = frame.Direction();
    x_dir  = frame.XDirection();
    break;
  case Section_plane::XZ:
    normal = frame.YDirection();
    x_dir  = frame.XDirection();
    break;
  case Section_plane::YZ:
    normal = frame.XDirection();
    x_dir  = frame.YDirection();
    break;
  }

  const gp_Pnt location = frame.Location().Translated(gp_Vec(normal) * offset);
  return gp_Pln(gp_Ax3(location, normal, x_dir));
}

Result<Section_geometry> section_shape_on_plane_(const TopoDS_Shape& shape, const gp_Pln& plane)
{
  if (shape.IsNull())
    return {Result_status::User_error, "Cannot section a null shape."};

  if (!contains_solid_(shape))
    return {Result_status::User_error, "Cross-section supports solid shapes only."};

  try
  {
    BRepAlgoAPI_Section section(shape, plane, false);
    section.Approximation(false);
    section.Build();
    if (!section.IsDone())
      return {Result_status::Topo_error, "Open CASCADE could not compute the section."};

    Section_geometry result;
    result.shape = section.Shape();
    for (TopExp_Explorer it(result.shape, TopAbs_EDGE); it.More(); it.Next())
    {
      ++result.edge_count;
      count_curve_type_(TopoDS::Edge(it.Current()), result);
    }

    if (result.shape.IsNull() || result.edge_count == 0)
      return {Result_status::User_error, "The section plane does not intersect the shape."};

    return result;
  }
  catch (const Standard_Failure& e)
  {
    return {Result_status::Topo_error, std::string("Section failed: ") + standard_failure_message(e)};
  }
}

void count_curve_type_(const TopoDS_Edge& edge, Section_geometry& result)
{
  switch (BRepAdaptor_Curve(edge).GetType())
  {
  case GeomAbs_Line:
    ++result.line_count;
    break;
  case GeomAbs_Circle:
    ++result.circle_count;
    break;
  case GeomAbs_Ellipse:
    ++result.ellipse_count;
    break;
  case GeomAbs_BSplineCurve:
    ++result.bspline_count;
    break;
  default:
    ++result.other_curve_count;
    break;
  }
}

bool contains_solid_(const TopoDS_Shape& shape)
{
  return !shape.IsNull() && (shape.ShapeType() == TopAbs_SOLID || TopExp_Explorer(shape, TopAbs_SOLID).More());
}

TopoDS_Shape shape_world_(const Shp& shp)
{
  TopoDS_Shape   shape           = shp.Shape();
  const gp_Trsf& local_transform = shp.LocalTransformation();
  if (local_transform.Form() != gp_Identity)
    shape = BRepBuilderAPI_Transform(shape, local_transform, true).Shape();

  return shape;
}

gp_Ax3 frame_world_(const Shp& shp)
{
  gp_Ax3         frame           = shp.get_frame();
  const gp_Trsf& local_transform = shp.LocalTransformation();
  if (local_transform.Form() != gp_Identity)
    frame.Transform(local_transform);

  return frame;
}

bool append_bounds_(const TopoDS_Shape& shape, Bnd_Box& bounds)
{
  Bnd_Box local;
  BRepBndLib::Add(shape, local);
  if (local.IsVoid())
    return false;

  bounds.Add(local);
  return true;
}

std::array<gp_Pnt, 8> bbox_corners_(const Bnd_Box& bounds)
{
  double x_min, y_min, z_min, x_max, y_max, z_max;
  bounds.Get(x_min, y_min, z_min, x_max, y_max, z_max);
  return {
      gp_Pnt(x_min, y_min, z_min), gp_Pnt(x_min, y_min, z_max), gp_Pnt(x_min, y_max, z_min), gp_Pnt(x_min, y_max, z_max),
      gp_Pnt(x_max, y_min, z_min), gp_Pnt(x_max, y_min, z_max), gp_Pnt(x_max, y_max, z_min), gp_Pnt(x_max, y_max, z_max),
  };
}

void project_bbox_offsets_(const Bnd_Box& bounds, const gp_Pnt& origin, const gp_Dir& normal, double& out_min, double& out_max)
{
  out_min = std::numeric_limits<double>::max();
  out_max = std::numeric_limits<double>::lowest();
  const gp_Vec normal_vec(normal);
  for (const gp_Pnt& corner : bbox_corners_(bounds))
  {
    const double t = gp_Vec(origin, corner).Dot(normal_vec);
    out_min        = std::min(out_min, t);
    out_max        = std::max(out_max, t);
  }
}

void add_plane_annotation_(const Bnd_Box& bounds, const gp_Pln& plane, BRep_Builder& builder, TopoDS_Compound& fill,
                           TopoDS_Compound& lines)
{
  if (bounds.IsVoid())
    return;

  const gp_Pnt origin = plane.Location();
  const gp_Vec x_axis(plane.XAxis().Direction());
  const gp_Vec y_axis(plane.YAxis().Direction());
  double       u_min = std::numeric_limits<double>::max();
  double       u_max = std::numeric_limits<double>::lowest();
  double       v_min = std::numeric_limits<double>::max();
  double       v_max = std::numeric_limits<double>::lowest();
  for (const gp_Pnt& corner : bbox_corners_(bounds))
  {
    const gp_Vec relative(origin, corner);
    const double u = relative.Dot(x_axis);
    const double v = relative.Dot(y_axis);
    u_min          = std::min(u_min, u);
    u_max          = std::max(u_max, u);
    v_min          = std::min(v_min, v);
    v_max          = std::max(v_max, v);
  }

  const double width  = u_max - u_min;
  const double height = v_max - v_min;
  const double margin = std::max(width, height) * 0.1;
  u_min -= margin;
  u_max += margin;
  v_min -= margin;
  v_max += margin;

  const TopoDS_Face plane_face = BRepBuilderAPI_MakeFace(plane, u_min, u_max, v_min, v_max).Face();
  builder.Add(fill, plane_face);

  auto point_on_plane = [&](double u, double v)
  {
    return origin.Translated(x_axis * u + y_axis * v);
  };

  BRepBuilderAPI_MakePolygon outline;
  outline.Add(point_on_plane(u_min, v_min));
  outline.Add(point_on_plane(u_max, v_min));
  outline.Add(point_on_plane(u_max, v_max));
  outline.Add(point_on_plane(u_min, v_max));
  outline.Close();
  builder.Add(lines, outline.Wire());

  const double center_u      = (u_min + u_max) * 0.5;
  const double center_v      = (v_min + v_max) * 0.5;
  const gp_Pnt center        = point_on_plane(center_u, center_v);
  const double normal_length = std::max(u_max - u_min, v_max - v_min) * 0.25;
  const gp_Vec normal(plane.Axis().Direction());
  const gp_Pnt tip = center.Translated(normal * normal_length);
  builder.Add(lines, BRepBuilderAPI_MakeEdge(center, tip).Edge());

  const gp_Pnt arrow_left  = tip.Translated(normal * (-normal_length * 0.2) + x_axis * (normal_length * 0.1));
  const gp_Pnt arrow_right = tip.Translated(normal * (-normal_length * 0.2) - x_axis * (normal_length * 0.1));
  builder.Add(lines, BRepBuilderAPI_MakeEdge(arrow_left, tip).Edge());
  builder.Add(lines, BRepBuilderAPI_MakeEdge(arrow_right, tip).Edge());
}
} // namespace
