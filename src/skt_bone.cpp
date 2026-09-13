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

// Bone outline (connecting-rod / dog-bone).
//
// Two end knobs sit on an axis. Two matching "cookie cutter" circles press in from
// opposite sides of that axis; each cutter kisses both knobs. The leftover meat
// between the cutters is the waist (skinniest neck).
//
// The sketch keeps only the trimmed outline: the outer cap of each knob plus the
// inner scoop of each cutter (four arcs). Full circles and the stadium side rails
// are construction only (Debug vis).
//
// Recipe: build a Bone_frame -> maybe solve cutter radius from waist -> place
// cutters (bone_geom_with_cut_radius_) -> walk the four join points (get_bone_profile)
// -> emit arcs (make_bone_wire).

namespace
{
// Shared 2d pose of the two knobs. Built once; cutter placement and the waist
// search reuse it so they do not re-derive the axis / similar-triangles bits.
struct Bone_frame
{
  gp_Pnt2d c1;
  gp_Pnt2d c2;
  double   r1 = 0;
  double   r2 = 0;
  gp_Vec2d axis;   // c1 -> c2 (not unit)
  gp_Pnt2d mid;    // midpoint of the knobs
  gp_Vec2d n;      // unit perp of the axis; + side hosts cut_plus
  double   ux = 0; // unit axis (c1 -> c2)
  double   uy = 0;
  // Stadium-rail tilt for unequal knobs (external common tangents).
  // Direction from each knob to its capsule contact is (ux, uy) rotated by an
  // angle whose cos is tan_cos and sin is tan_sin.
  double tan_cos = 0; // (r1 - r2) / |axis|
  double tan_sin = 0; // sqrt(1 - tan_cos^2)
};

std::optional<Bone_frame> make_bone_frame_(const Bone_params& p);
std::optional<Bone_geom>  bone_geom_with_cut_radius_(const Bone_frame& f, double cut_r);
std::optional<double>     solve_cut_radius_for_waist_(const Bone_frame& f, double target_waist);
gp_Pnt2d                  mirror_across_axis_(const gp_Pnt2d& p, const gp_Pnt2d& c1, const gp_Vec2d& axis);
std::optional<gp_Pnt2d>   circle_circle_intersect_pick_side_(const gp_Pnt2d& c1, double ra, const gp_Pnt2d& c2, double rb,
                                                             const gp_Pnt2d& mid, const gp_Vec2d& n, bool positive_n_side);
double                    bone_waist_gap_(const gp_Pnt2d& c_top, const gp_Pnt2d& c_bot, double cut_r);
#if DEV_MODE
void add_debug_seg_(TopoDS_Compound& comp, BRep_Builder& bb, const gp_Pln& pln, const gp_Pnt2d& a, const gp_Pnt2d& b);
void add_debug_circle_(TopoDS_Compound& comp, BRep_Builder& bb, const gp_Pln& pln, const gp_Pnt2d& c, double r);
#endif
} // namespace

std::optional<Bone_geom> compute_bone_geom(const Bone_params& p)
{
  const double eps = Precision::Confusion();
  if (p.drive == Bone_drive::Cut_radius && p.cut_radius <= eps)
    return std::nullopt;

  if (p.drive == Bone_drive::Waist && p.waist <= eps)
    return std::nullopt;

  const std::optional<Bone_frame> frame = make_bone_frame_(p);
  if (!frame)
    return std::nullopt;

  double cut_r = p.cut_radius;
  if (p.drive == Bone_drive::Waist)
  {
    const std::optional<double> solved = solve_cut_radius_for_waist_(*frame, p.waist);
    if (!solved)
      return std::nullopt;

    cut_r = *solved;
  }

  return bone_geom_with_cut_radius_(*frame, cut_r);
}

Bone_profile get_bone_profile(const Bone_geom& g)
{
  gp_Vec2d     axis(g.c1, g.c2);
  const double dist = axis.Magnitude();
  EZY_ASSERT(dist > Precision::Confusion());
  const gp_Vec2d ad(axis / dist);

  // A cutter that kisses a knob is externally tangent, so cutter center, contact,
  // and knob center sit on one line. Walk from the knob toward the cutter a
  // distance r (equivalently, cut_radius back from the cutter).
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
  // Outer poles: points on each knob facing away from the other, on the bone axis.
  p.c1_outer = gp_Pnt2d(g.c1).Translated(-ad * g.r1);
  p.c2_outer = gp_Pnt2d(g.c2).Translated(ad * g.r2);

  // Neck dimple: where the two cutters come closest. When the knobs differ in
  // size this is not at the midpoint; it slides toward the smaller knob.
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

  // Walk the outline: outer cap of knob 1, + cutter scoop, outer cap of knob 2,
  // - cutter scoop. Join points are the four kisses (Debug vis cutter radials).
  BRepBuilderAPI_MakeWire wire;
  wire.Add(arc(p.c1_minus, p.c1_outer, p.c1_plus));
  wire.Add(arc(p.c1_plus, p.waist_plus, p.c2_plus));
  wire.Add(arc(p.c2_plus, p.c2_outer, p.c2_minus));
  wire.Add(arc(p.c2_minus, p.waist_minus, p.c1_minus));
  return wire.Wire();
}

TopoDS_Shape make_bone_preview_shape(const gp_Pln& pln, const Bone_geom& g) { return make_bone_wire(pln, g); }

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

  auto mark = [&]()
  {
    any = true;
  };

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

  // Straight stadium rails of the two knobs alone (not part of the trimmed outline).
  if (flags.capsule_tangents)
  {
    add_debug_seg_(comp, bb, pln, g.tan_top_a, g.tan_top_b);
    add_debug_seg_(comp, bb, pln, g.tan_bot_a, g.tan_bot_b);
    mark();
  }

  // Four spokes: cutter center to the kiss on that knob. Continuing the same line
  // would hit the knob center (cutter, contact, and knob are collinear). The
  // committed outline switches from end-cap arc to waist arc at that contact.
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

namespace
{
std::optional<Bone_frame> make_bone_frame_(const Bone_params& p)
{
  const double eps = Precision::Confusion();
  if (p.r1 <= eps || p.r2 <= eps)
    return std::nullopt;

  Bone_frame f;
  f.c1   = p.c1;
  f.c2   = p.c2;
  f.r1   = p.r1;
  f.r2   = p.r2;
  f.axis = gp_Vec2d(p.c1, p.c2);

  const double dist = f.axis.Magnitude();
  if (dist <= eps)
    return std::nullopt;

  // One knob swallowed by the other: no stadium, and no cutter can kiss both rims.
  if (dist + eps < std::fabs(p.r1 - p.r2))
    return std::nullopt;

  f.ux      = f.axis.X() / dist;
  f.uy      = f.axis.Y() / dist;
  f.tan_cos = (p.r1 - p.r2) / dist;

  const double sin2 = 1.0 - f.tan_cos * f.tan_cos;
  if (sin2 < -eps)
    return std::nullopt;

  f.tan_sin = std::sqrt(std::max(0.0, sin2));
  f.mid     = get_midpoint(p.c1, p.c2);
  f.n       = gp_Vec2d(f.ux, f.uy).Rotated(std::numbers::pi / 2.0);
  return f;
}

// Fold p over the bone axis (the crease through c1 along axis), like mirroring
// across a sheet of paper. The second cutter is the twin of the first.
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

// Crossing of two rings. Walk from c1 along the line of centers to the chord
// that joins the two crossings (distance `along`), then step sideways by `drop`
// (the height of that isosceles tent). The two landings are mirrored across the
// line of centers; keep the one on the requested side of `n` through `mid`.
//
// Null when the rings are separate, nested without touching, or degenerate.
std::optional<gp_Pnt2d> circle_circle_intersect_pick_side_(const gp_Pnt2d& c1, double ra, const gp_Pnt2d& c2, double rb,
                                                           const gp_Pnt2d& mid, const gp_Vec2d& n, bool positive_n_side)
{
  const double eps = Precision::Confusion();
  gp_Vec2d     axis(c1, c2);
  const double d = axis.Magnitude();
  if (d <= eps || ra <= eps || rb <= eps)
    return std::nullopt;

  // Separate rings, or one nested inside the other without touching.
  if (d > ra + rb + eps || d + eps < std::fabs(ra - rb))
    return std::nullopt;

  const double along = (ra * ra - rb * rb + d * d) / (2.0 * d);
  const double drop2 = ra * ra - along * along;
  if (drop2 < -eps)
    return std::nullopt;

  const double   drop = std::sqrt(std::max(0.0, drop2));
  const gp_Vec2d ad(axis / d);
  gp_Pnt2d       chord = gp_Pnt2d(c1).Translated(ad * along);
  gp_Vec2d       sideways(-ad.Y() * drop, ad.X() * drop);

  const gp_Pnt2d i0 = chord.Translated(sideways);
  const gp_Pnt2d i1 = chord.Translated(-sideways);

  const double d0 = gp_Vec2d(mid, i0).Dot(n);
  const double d1 = gp_Vec2d(mid, i1).Dot(n);
  if (positive_n_side)
    return d0 >= d1 ? i0 : i1;

  return d0 <= d1 ? i0 : i1;
}

double bone_waist_gap_(const gp_Pnt2d& c_top, const gp_Pnt2d& c_bot, double cut_r)
{
  // Leftover meat: distance between cutter centers minus both radii.
  return c_top.Distance(c_bot) - 2.0 * cut_r;
}

// Place both cutters for a known cookie-cutter radius. The waist search calls
// this as an oracle (many trial radii); compute_bone_geom calls it once for the
// result. Cannot go through compute_bone_geom: that entry may still need to solve
// cut_r from waist.
std::optional<Bone_geom> bone_geom_with_cut_radius_(const Bone_frame& f, double cut_r)
{
  const double eps = Precision::Confusion();
  if (cut_r <= eps)
    return std::nullopt;

  // A cutter that kisses both knobs has its center at a point that is
  // (cut_r + r1) from c1 and (cut_r + r2) from c2: the crossing of two rings
  // grown out from each knob's rim. Pick the + side of the bone axis; mirror
  // for the twin. The outline later meets each knob on the line of those centers.
  const std::optional<gp_Pnt2d> c_top =
      circle_circle_intersect_pick_side_(f.c1, cut_r + f.r1, f.c2, cut_r + f.r2, f.mid, f.n, true);
  if (!c_top)
    return std::nullopt;

  Bone_geom g;
  g.c1         = f.c1;
  g.c2         = f.c2;
  g.r1         = f.r1;
  g.r2         = f.r2;
  g.cut_radius = cut_r;
  g.cut_plus   = *c_top;
  g.cut_minus  = mirror_across_axis_(*c_top, f.c1, f.axis);
  g.waist      = bone_waist_gap_(g.cut_plus, g.cut_minus, cut_r);

  // Stadium rails of the knobs alone (Debug vis). Same contact direction on both
  // circles keeps the radii parallel, so the segment between contacts is tangent
  // to both. sign picks which side of the axis.
  auto tangent_pair = [&](double sign, gp_Pnt2d& t1, gp_Pnt2d& t2)
  {
    const double nx = f.ux * f.tan_cos - sign * f.uy * f.tan_sin;
    const double ny = f.uy * f.tan_cos + sign * f.ux * f.tan_sin;
    t1              = gp_Pnt2d(f.c1.X() + f.r1 * nx, f.c1.Y() + f.r1 * ny);
    t2              = gp_Pnt2d(f.c2.X() + f.r2 * nx, f.c2.Y() + f.r2 * ny);
  };

  tangent_pair(1.0, g.tan_top_a, g.tan_top_b);
  tangent_pair(-1.0, g.tan_bot_a, g.tan_bot_b);
  return g;
}

// Find the cookie-cutter radius that leaves a leftover neck of width target_waist.
//
// Picture a dog bone: two end knobs (circles r1, r2) on an axis. Two matching cutter
// circles press in from opposite sides, each kissing both knobs. The waist is the
// leftover meat between those cutters - the skinniest gap.
//
// A small cutter nestles into the notch between the knobs and takes a deep bite
// (skinny neck). A large cutter sits farther off-axis, like a giant coin grazing
// the knobs, so the bite is shallower and more neck remains. Bigger cutter -> fatter
// waist. Search for the radius whose leftover gap matches the target.
std::optional<double> solve_cut_radius_for_waist_(const Bone_frame& f, double target_waist)
{
  const double eps = Precision::Confusion();
  if (target_waist <= eps)
    return std::nullopt;

  auto leftover_neck = [&](double cut_r) -> double
  {
    const std::optional<Bone_geom> g = bone_geom_with_cut_radius_(f, cut_r);
    if (!g)
      return std::numeric_limits<double>::quiet_NaN();

    return g->waist;
  };

  const double dist = f.axis.Magnitude();
  // Tiniest cutter that can still kiss both knobs: half the leftover axis after
  // laying the two end circles end-to-end. Any smaller and it cannot reach both.
  double r_lo = (dist - f.r1 - f.r2) / 2.0;
  if (r_lo <= eps)
    r_lo = eps;

  const double w_lo = leftover_neck(r_lo);
  if (!std::isfinite(w_lo))
    return std::nullopt;

  // Even that deepest bite already left a thicker neck than asked. The target is too
  // skinny: it would need a cutter that cannot reach both knobs.
  if (w_lo >= target_waist)
    return std::nullopt;

  // Inflate the cutter (double the radius) until the bite is shallow enough that the
  // leftover neck is at least the target. Start from a guess on the order of the knobs.
  double r_hi = std::max(r_lo + 1.0, f.r1 + f.r2);
  for (int i = 0; i < 48; ++i)
  {
    const double w_hi = leftover_neck(r_hi);
    if (!std::isfinite(w_hi) || w_hi < target_waist)
      r_hi *= 2.0; // still too pinched; grow the scoop
    else
      break;
  }

  const double w_hi = leftover_neck(r_hi);
  if (!std::isfinite(w_hi) || w_hi < target_waist)
    return std::nullopt;

  // Close the calipers: bisect cutter radius until leftover waist matches.
  for (int i = 0; i < 64; ++i)
  {
    const double r_mid = (r_lo + r_hi) * 0.5;
    const double w_mid = leftover_neck(r_mid);
    if (!std::isfinite(w_mid))
      return std::nullopt;

    if (w_mid < target_waist)
      r_lo = r_mid; // still too skinny; need a bigger (shallower) cutter
    else
      r_hi = r_mid; // too fat; shrink the cutter to pinch more
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
