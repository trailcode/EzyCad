#include "skt_bone.h"

#include "utl_dbg.h"
#include "utl_geom.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRep_Builder.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Precision.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <algorithm>
#include <cmath>
#include <gp_Pln.hxx>
#include <gp_Vec2d.hxx>
#include <limits>
#include <numbers>

namespace
{
gp_Pnt2d mirror_across_axis_(const gp_Pnt2d& p, const gp_Pnt2d& c1, const gp_Vec2d& axis)
{
  const double len = axis.Magnitude();
  if (len <= Precision::Confusion())
    return p;

  const gp_Vec2d a(axis / len);
  gp_Vec2d       v(c1, p);
  const double   along = v.Dot(a);
  gp_Vec2d       perp  = v - a * along;
  return gp_Pnt2d(c1).Translated(a * along - perp);
}

std::optional<gp_Pnt2d> circle_circle_intersect_pick_side_(const gp_Pnt2d& c1, double ra, const gp_Pnt2d& c2, double rb,
                                                           const gp_Pnt2d& mid, const gp_Vec2d& n, bool positive_n_side)
{
  const double eps = Precision::Confusion();
  gp_Vec2d     axis(c1, c2);
  const double d = axis.Magnitude();
  if (d <= eps || ra <= eps || rb <= eps)
    return std::nullopt;

  if (d > ra + rb + eps || d + eps < std::fabs(ra - rb))
    return std::nullopt;

  const double a  = (ra * ra - rb * rb + d * d) / (2.0 * d);
  const double h2 = ra * ra - a * a;
  if (h2 < -eps)
    return std::nullopt;

  const double   h = std::sqrt(std::max(0.0, h2));
  const gp_Vec2d ad(axis / d);
  gp_Pnt2d       p2 = gp_Pnt2d(c1).Translated(ad * a);
  gp_Vec2d       perp(-ad.Y() * h, ad.X() * h);

  const gp_Pnt2d i0 = p2.Translated(perp);
  const gp_Pnt2d i1 = p2.Translated(-perp);

  const double d0 = gp_Vec2d(mid, i0).Dot(n);
  const double d1 = gp_Vec2d(mid, i1).Dot(n);
  if (positive_n_side)
    return d0 >= d1 ? i0 : i1;
  return d0 <= d1 ? i0 : i1;
}

double bone_waist_gap_(const gp_Pnt2d& c_top, const gp_Pnt2d& c_bot, double cut_r)
{
  // Min gap between the two cutter circles (along the line of centers).
  return c_top.Distance(c_bot) - 2.0 * cut_r;
}

std::optional<Bone_geom> bone_geom_with_cut_radius_(const gp_Pnt2d& c1, const gp_Pnt2d& c2, double r1, double r2,
                                                    double cut_r, const gp_Vec2d& axis, const gp_Vec2d& n,
                                                    const gp_Pnt2d& mid, double vx, double vy, double a, double h)
{
  const double eps = Precision::Confusion();
  if (cut_r <= eps)
    return std::nullopt;

  // External tangency with both ends: cutter center lies on the circle of radius (cut_r + r1)
  // around c1 and (cut_r + r2) around c2. That intersection is the cutter center. The bone
  // outline later meets each end circle on the line of those centers (see get_bone_profile).
  const std::optional<gp_Pnt2d> c_top =
      circle_circle_intersect_pick_side_(c1, cut_r + r1, c2, cut_r + r2, mid, n, true);
  if (!c_top)
    return std::nullopt;

  Bone_geom g;
  g.c1         = c1;
  g.c2         = c2;
  g.r1         = r1;
  g.r2         = r2;
  g.cut_radius = cut_r;
  g.cut_plus   = *c_top;
  g.cut_minus  = mirror_across_axis_(*c_top, c1, axis);
  g.waist      = bone_waist_gap_(g.cut_plus, g.cut_minus, cut_r);

  auto tangent_pair = [&](double sign, gp_Pnt2d& t1, gp_Pnt2d& t2)
  {
    const double nx = vx * a - sign * vy * h;
    const double ny = vy * a + sign * vx * h;
    t1              = gp_Pnt2d(c1.X() + r1 * nx, c1.Y() + r1 * ny);
    t2              = gp_Pnt2d(c2.X() + r2 * nx, c2.Y() + r2 * ny);
  };

  tangent_pair(1.0, g.tan_top_a, g.tan_top_b);
  tangent_pair(-1.0, g.tan_bot_a, g.tan_bot_b);
  return g;
}

double bone_waist_for_cut_radius_(const gp_Pnt2d& c1, const gp_Pnt2d& c2, double r1, double r2, double cut_r,
                                  const gp_Vec2d& axis, const gp_Vec2d& n, const gp_Pnt2d& mid, double vx, double vy,
                                  double a, double h)
{
  const std::optional<Bone_geom> g = bone_geom_with_cut_radius_(c1, c2, r1, r2, cut_r, axis, n, mid, vx, vy, a, h);
  if (!g)
    return std::numeric_limits<double>::quiet_NaN();
  return g->waist;
}

std::optional<double> solve_cut_radius_for_waist_(const gp_Pnt2d& c1, const gp_Pnt2d& c2, double r1, double r2,
                                                  double target_waist, const gp_Vec2d& axis, const gp_Vec2d& n,
                                                  const gp_Pnt2d& mid, double vx, double vy, double a, double h)
{
  const double eps = Precision::Confusion();
  if (target_waist <= eps)
    return std::nullopt;

  const double dist = axis.Magnitude();
  double       r_lo = (dist - r1 - r2) / 2.0;
  if (r_lo <= eps)
    r_lo = eps;

  double w_lo = bone_waist_for_cut_radius_(c1, c2, r1, r2, r_lo, axis, n, mid, vx, vy, a, h);
  if (!std::isfinite(w_lo))
    return std::nullopt;

  if (w_lo >= target_waist)
    return std::nullopt;

  double r_hi = std::max(r_lo + 1.0, r1 + r2);
  for (int i = 0; i < 48; ++i)
  {
    const double w_hi = bone_waist_for_cut_radius_(c1, c2, r1, r2, r_hi, axis, n, mid, vx, vy, a, h);
    if (!std::isfinite(w_hi) || w_hi < target_waist)
      r_hi *= 2.0;
    else
      break;
  }

  const double w_hi = bone_waist_for_cut_radius_(c1, c2, r1, r2, r_hi, axis, n, mid, vx, vy, a, h);
  if (!std::isfinite(w_hi) || w_hi < target_waist)
    return std::nullopt;

  for (int i = 0; i < 64; ++i)
  {
    const double r_mid = (r_lo + r_hi) * 0.5;
    const double w_mid = bone_waist_for_cut_radius_(c1, c2, r1, r2, r_mid, axis, n, mid, vx, vy, a, h);
    if (!std::isfinite(w_mid))
      return std::nullopt;

    if (w_mid < target_waist)
      r_lo = r_mid;
    else
      r_hi = r_mid;
  }

  return (r_lo + r_hi) * 0.5;
}

#if DEV_MODE
void add_debug_seg_(TopoDS_Compound& comp, BRep_Builder& bb, const gp_Pln& pln, const gp_Pnt2d& a, const gp_Pnt2d& b)
{
  if (a.Distance(b) <= Precision::Confusion())
    return;

  bb.Add(comp, BRepBuilderAPI_MakeEdge(to_3d(pln, a), to_3d(pln, b)).Edge());
}

void add_debug_circle_(TopoDS_Compound& comp, BRep_Builder& bb, const gp_Pln& pln, const gp_Pnt2d& c, double r)
{
  if (r <= Precision::Confusion())
    return;

  bb.Add(comp, make_circle_wire(pln, c, gp_Pnt2d(c.X() + r, c.Y())));
}
#endif
} // namespace

std::optional<Bone_geom> compute_bone_geom(const Bone_params& p)
{
  const double eps = Precision::Confusion();
  if (p.r1 <= eps || p.r2 <= eps)
    return std::nullopt;

  if (p.drive == Bone_drive::Cut_radius && p.cut_radius <= eps)
    return std::nullopt;
  if (p.drive == Bone_drive::Waist && p.waist <= eps)
    return std::nullopt;

  const gp_Vec2d axis(p.c1, p.c2);
  const double   dist = axis.Magnitude();
  if (dist <= eps)
    return std::nullopt;

  if (dist + eps < std::fabs(p.r1 - p.r2))
    return std::nullopt;

  const double   vx = axis.X() / dist;
  const double   vy = axis.Y() / dist;
  const double   a  = (p.r1 - p.r2) / dist;
  const double   h2 = 1.0 - a * a;
  if (h2 < -eps)
    return std::nullopt;

  const double   h   = std::sqrt(std::max(0.0, h2));
  const gp_Pnt2d mid = get_midpoint(p.c1, p.c2);
  const gp_Vec2d n   = gp_Vec2d(vx, vy).Rotated(std::numbers::pi / 2.0);

  double cut_r = p.cut_radius;
  if (p.drive == Bone_drive::Waist)
  {
    const std::optional<double> solved =
        solve_cut_radius_for_waist_(p.c1, p.c2, p.r1, p.r2, p.waist, axis, n, mid, vx, vy, a, h);
    if (!solved)
      return std::nullopt;
    cut_r = *solved;
  }

  return bone_geom_with_cut_radius_(p.c1, p.c2, p.r1, p.r2, cut_r, axis, n, mid, vx, vy, a, h);
}

Bone_profile get_bone_profile(const Bone_geom& g)
{
  gp_Vec2d     axis(g.c1, g.c2);
  const double dist = axis.Magnitude();
  EZY_ASSERT(dist > Precision::Confusion());
  const gp_Vec2d ad(axis / dist);

  // External tangency of end circle (c, r) with cutter (cut, cut_radius):
  // |c - cut| = r + cut_radius, so the contact sits on the line of centers, a fraction
  // r / |c - cut| from the end center toward the cutter (equivalently cut_radius from the cutter).
  auto contact = [](const gp_Pnt2d& c, double r, const gp_Pnt2d& cut) -> gp_Pnt2d
  {
    gp_Vec2d     v(c, cut);
    const double d = v.Magnitude();
    EZY_ASSERT(d > Precision::Confusion());
    return gp_Pnt2d(c).Translated(v * (r / d));
  };

  Bone_profile p;
  p.c1_plus  = contact(g.c1, g.r1, g.cut_plus);
  p.c1_minus = contact(g.c1, g.r1, g.cut_minus);
  p.c2_plus  = contact(g.c2, g.r2, g.cut_plus);
  p.c2_minus = contact(g.c2, g.r2, g.cut_minus);
  p.c1_outer = gp_Pnt2d(g.c1).Translated(-ad * g.r1);
  p.c2_outer = gp_Pnt2d(g.c2).Translated(ad * g.r2);

  // Skinniest section: points of closest approach between the two cutter circles.
  // When r1 != r2 this is offset along the bone axis from the center midpoint.
  gp_Vec2d     to_minus(g.cut_plus, g.cut_minus);
  const double cut_sep = to_minus.Magnitude();
  EZY_ASSERT(cut_sep > Precision::Confusion());
  to_minus /= cut_sep;
  p.waist_plus  = gp_Pnt2d(g.cut_plus).Translated(to_minus * g.cut_radius);
  p.waist_minus = gp_Pnt2d(g.cut_minus).Translated(-to_minus * g.cut_radius);
  return p;
}

TopoDS_Wire make_bone_wire(const gp_Pln& pln, const Bone_geom& g)
{
  const Bone_profile p = get_bone_profile(g);

  auto arc = [&](const gp_Pnt2d& a, const gp_Pnt2d& mid, const gp_Pnt2d& b) -> TopoDS_Edge
  {
    GC_MakeArcOfCircle maker(to_3d(pln, a), to_3d(pln, mid), to_3d(pln, b));
    return BRepBuilderAPI_MakeEdge(maker.Value()).Edge();
  };

  BRepBuilderAPI_MakeWire wire;
  // End-cap arcs (outer poles) then waist arcs. Join points are the cutter/end contacts
  // (the far ends of the Debug vis cutter radials).
  wire.Add(arc(p.c1_minus, p.c1_outer, p.c1_plus));
  wire.Add(arc(p.c1_plus, p.waist_plus, p.c2_plus));
  wire.Add(arc(p.c2_plus, p.c2_outer, p.c2_minus));
  wire.Add(arc(p.c2_minus, p.waist_minus, p.c1_minus));
  return wire.Wire();
}

TopoDS_Shape make_bone_preview_shape(const gp_Pln& pln, const Bone_geom& g)
{
  return make_bone_wire(pln, g);
}

#if DEV_MODE
std::optional<TopoDS_Shape> make_bone_debug_shape(const gp_Pln& pln, const Bone_geom& g, const Bone_debug_flags& flags)
{
  if (!flags.cut_circles && !flags.end_circles && !flags.capsule_tangents && !flags.cutter_centers && !flags.contacts &&
      !flags.cutter_radials)
    return std::nullopt;

  const Bone_profile pr = get_bone_profile(g);

  TopoDS_Compound comp;
  BRep_Builder    bb;
  bb.MakeCompound(comp);
  bool any = false;

  auto mark = [&]() { any = true; };

  if (flags.cut_circles)
  {
    add_debug_circle_(comp, bb, pln, g.cut_plus, g.cut_radius);
    add_debug_circle_(comp, bb, pln, g.cut_minus, g.cut_radius);
    mark();
  }

  if (flags.end_circles)
  {
    add_debug_circle_(comp, bb, pln, g.c1, g.r1);
    add_debug_circle_(comp, bb, pln, g.c2, g.r2);
    mark();
  }

  if (flags.capsule_tangents)
  {
    add_debug_seg_(comp, bb, pln, g.tan_top_a, g.tan_top_b);
    add_debug_seg_(comp, bb, pln, g.tan_bot_a, g.tan_bot_b);
    mark();
  }

  // Four spokes, one per (cutter, end) pair. Each is the cutter radius to the tangency
  // with that end circle. Continuing the same line past the contact would hit the end
  // center: cutter, contact, and end center are collinear. The committed outline switches
  // from end-cap arc to waist arc at that contact.
  if (flags.cutter_radials)
  {
    add_debug_seg_(comp, bb, pln, g.cut_plus, pr.c1_plus);
    add_debug_seg_(comp, bb, pln, g.cut_plus, pr.c2_plus);
    add_debug_seg_(comp, bb, pln, g.cut_minus, pr.c1_minus);
    add_debug_seg_(comp, bb, pln, g.cut_minus, pr.c2_minus);
    mark();
  }

  const double half = std::max(g.cut_radius, std::max(g.r1, g.r2)) * 0.08;
  auto         plus = [&](const gp_Pnt2d& p)
  {
    bb.Add(comp, create_plus_cross_shape(pln, to_3d(pln, p), std::max(half, Precision::Confusion() * 50.0)));
    mark();
  };

  if (flags.cutter_centers)
  {
    plus(g.cut_plus);
    plus(g.cut_minus);
  }

  if (flags.contacts)
  {
    plus(pr.c1_plus);
    plus(pr.c1_minus);
    plus(pr.c2_plus);
    plus(pr.c2_minus);
    plus(pr.waist_plus);
    plus(pr.waist_minus);
  }

  if (!any)
    return std::nullopt;

  return comp;
}
#endif
