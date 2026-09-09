#pragma once

#include <gp_Pnt2d.hxx>
#include <optional>

class gp_Pln;
class TopoDS_Shape;
class TopoDS_Wire;

/// Which dialog value drives the waist cutter radius (the other is derived).
enum class Bone_drive
{
  Cut_radius,
  Waist,
};

/// Two end circles, two waist cutters, and the capsule external tangents.
struct Bone_params
{
  gp_Pnt2d   c1;
  gp_Pnt2d   c2;
  double     r1{0};
  double     r2{0};
  double     cut_radius{0};
  double     waist{0};
  Bone_drive drive{Bone_drive::Waist};
};

struct Bone_geom
{
  gp_Pnt2d c1;
  gp_Pnt2d c2;
  gp_Pnt2d cut_plus;
  gp_Pnt2d cut_minus;
  double   r1{0};
  double   r2{0};
  double   cut_radius{0};
  double   waist{0};
  gp_Pnt2d tan_top_a;
  gp_Pnt2d tan_top_b;
  gp_Pnt2d tan_bot_a;
  gp_Pnt2d tan_bot_b;
};

/// Trimmed bone outline: outer end-circle caps and inner waist arcs (no full circles, no capsule tangents).
struct Bone_profile
{
  gp_Pnt2d c1_plus;   // contact, end 1 / waist cutter +
  gp_Pnt2d c1_outer;  // outer pole of end 1
  gp_Pnt2d c1_minus;  // contact, end 1 / waist cutter -
  gp_Pnt2d c2_plus;
  gp_Pnt2d c2_outer;
  gp_Pnt2d c2_minus;
  gp_Pnt2d waist_plus;  // inner bulge of cutter +
  gp_Pnt2d waist_minus;
};

/// Null when centers coincide, a circle is inside the other, or waist/cut radius cannot be solved.
std::optional<Bone_geom> compute_bone_geom(const Bone_params& p);

Bone_profile get_bone_profile(const Bone_geom& g);

/// Closed wire of the four outline arcs (preview and extrusion profile).
TopoDS_Wire make_bone_wire(const gp_Pln& pln, const Bone_geom& g);

/// Same as \\a make_bone_wire (AIS preview).
TopoDS_Shape make_bone_preview_shape(const gp_Pln& pln, const Bone_geom& g);
