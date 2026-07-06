#include "sketch_edge.h"

#include <BRepAdaptor_Curve.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <algorithm>
#include <cmath>

#include "utl.h"
#include "utl_geom.h"

bool Sketch_edge::reversed(size_t idx_a, size_t idx_b) const
{
  if (node_idx_a == idx_a && node_idx_b == idx_b)
    return false;

  if (node_idx_a == idx_b && node_idx_b == idx_a)
    return true;

  EZY_ASSERT(false);
  return false;
}

bool sketch_edge_is_arc(const Sketch_edge& e)
{
  if (!e.node_idx_b.has_value() || e.shp.IsNull())
    return false;

  const BRepAdaptor_Curve curve(TopoDS::Edge(e.shp->Shape()));
  return curve.GetType() == GeomAbs_Circle;
}

bool sketch_edge_is_linear(const Sketch_edge& e) { return e.node_idx_b.has_value() && !sketch_edge_is_arc(e); }

namespace
{
gp_Vec2d normalize_or_axis_(gp_Vec2d v)
{
  const double mag2 = v.SquareMagnitude();
  if (mag2 < Precision::Confusion() * Precision::Confusion())
    return gp_Vec2d(1.0, 0.0);

  v.Normalize();
  return v;
}

gp_Vec2d curve_tangent_dir_2d_(const BRepAdaptor_Curve& curve, double u, const gp_Pln& pln, bool forward)
{
  gp_Pnt p;
  gp_Vec tan3;
  curve.D1(u, p, tan3);
  if (!forward)
    tan3.Reverse();

  const gp_Dir& x_dir = pln.XAxis().Direction();
  const gp_Dir& y_dir = pln.YAxis().Direction();
  return normalize_or_axis_(gp_Vec2d(tan3.Dot(gp_Vec(x_dir)), tan3.Dot(gp_Vec(y_dir))));
}

gp_Vec2d arc_outgoing_dir_2d_(const Sketch_edge& e, const gp_Pnt2d& from_pt, const gp_Pnt2d& to_pt, const gp_Pln& pln)
{
  EZY_ASSERT(!e.shp.IsNull());
  const TopoDS_Edge       edge = TopoDS::Edge(e.shp->Shape());
  const BRepAdaptor_Curve curve(edge);
  const gp_Pnt            from_3d  = to_3d(pln, from_pt);
  const gp_Pnt            to_pt_3d = to_3d(pln, to_pt);

  const Handle(Geom_Curve) geom = curve.Curve().Curve();
  const double u_first          = curve.FirstParameter();
  const double u_last           = curve.LastParameter();

  GeomAPI_ProjectPointOnCurve proj_from(from_3d, geom, u_first, u_last);
  GeomAPI_ProjectPointOnCurve proj_to(to_pt_3d, geom, u_first, u_last);
  EZY_ASSERT(proj_from.NbPoints() > 0 && proj_to.NbPoints() > 0);

  const double u_from = proj_from.LowerDistanceParameter();
  const double u_to   = proj_to.LowerDistanceParameter();
  const double du     = u_to - u_from;

  if (std::abs(du) < Precision::Confusion())
    return curve_tangent_dir_2d_(curve, u_from, pln, true);

  const double span   = u_last - u_first;
  const double step   = std::max(std::abs(du) * 0.01, span * 1e-4);
  double       u_step = u_from + (du > 0.0 ? step : -step);
  u_step              = std::clamp(u_step, u_first, u_last);

  gp_Vec2d ret(from_pt, to_2d(pln, curve.Value(u_step)));
  if (ret.SquareMagnitude() < Precision::Confusion() * Precision::Confusion())
    return curve_tangent_dir_2d_(curve, u_from, pln, du > 0.0);

  return normalize_or_axis_(ret);
}
} // namespace

gp_Vec2d sketch_edge_outgoing_dir_2d(const Sketch_edge& e, const gp_Pnt2d& from_pt, const gp_Pnt2d& to_pt, const gp_Pln& pln)
{
  if (sketch_edge_is_arc(e))
    return arc_outgoing_dir_2d_(e, from_pt, to_pt, pln);

  return normalize_or_axis_(gp_Vec2d(from_pt, to_pt));
}
