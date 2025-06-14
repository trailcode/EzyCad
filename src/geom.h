#pragma once

#include <Precision.hxx>
#include <Standard_Handle.hxx>
#include <algorithm>
#include <array>
#include <boost/geometry.hpp>
#include <glm/glm.hpp>
#include <gp_Dir2d.hxx>
#include <optional>
#include <numbers>

#include "dbg.h"
#include "types.h"

class gp_Pln;
class gp_Pnt;
class gp_Pnt2d;
class gp_Vec;
class TopoDS_Edge;
class TopoDS_Wire;
class TopoDS_Face;
class TopoDS_Shape;
class Geom_TrimmedCurve;

namespace boost_geom
{
typedef boost::geometry::model::d2::point_xy<double> point_2d;
typedef boost::geometry::model::ring<point_2d>       ring_2d;
typedef boost::geometry::model::polygon<point_2d>    polygon_2d;
typedef boost::geometry::model::linestring<point_2d> linestring_2d;
}  // namespace boost_geom

// Function to project a 3D point onto a plane and get its 2D (u, v) coordinates
gp_Pnt2d to_2d(const gp_Pln& plane, const gp_Pnt& point_3d);

// Convert 2D (u, v) point on gp_Pln to 3D point
gp_Pnt to_3d(const gp_Pln& plane, const gp_Pnt2d& point_2d);

gp_Pnt2d to_pnt2d(const boost_geom::point_2d& pt);

// Function to create a wire box centered on a point on a plane, returning a TopoDS_Wire
TopoDS_Wire create_wire_box(const gp_Pln& plane, const gp_Pnt& position, double width, double height);

TopoDS_Wire make_square_wire(const gp_Pln&   pln,
                             const gp_Pnt2d& center,
                             const gp_Pnt2d& edge_midpoint);

std::array<gp_Pnt2d, 4> square_corners(const gp_Pnt2d& center, const gp_Pnt2d& edge_midpoint);

std::array<gp_Pnt2d, 4> xy_stencil_pnts(const gp_Pnt2d& center, const gp_Pnt2d& edge_midpoint);

TopoDS_Wire make_circle_wire(const gp_Pln&   pln,
                             const gp_Pnt2d& center,
                             const gp_Pnt2d& edge_point);

struct Slot_pnts
{
  gp_Pnt2d a_top_2d;
  gp_Pnt2d a_mid_2d;
  gp_Pnt2d a_bottom_2d;

  gp_Pnt2d b_bottom_2d;
  gp_Pnt2d b_mid_2d;
  gp_Pnt2d b_top_2d;
};

Slot_pnts get_slot_points(const gp_Pnt2d& pt_a,
                          const gp_Pnt2d& pt_b,
                          const gp_Pnt2d& pt_c);

TopoDS_Wire make_slot_wire(const gp_Pln&   plane,
                           const gp_Pnt2d& pt_a,
                           const gp_Pnt2d& pt_b,
                           const gp_Pnt2d& pt_c);

// Function to compute angle between two vectors (in radians, 0 to 2pi)
// TODO not used, there must be a OCCT func.
#if 0
double compute_angle(const gp_Vec2d& v1, const gp_Vec2d& v2)
{
  double dot   = v1.Dot(v2);
  double det   = v1.X() * v2.Y() - v1.Y() * v2.X();  // Cross product in 2D
  double angle = std::atan2(det, dot);
  return (angle < 0) ? angle + 2 * std::numbers::pi : angle;  // Normalize to [0, 2pi]
}
#endif

// Function to get the directional vectors at the start and end of a Geom_TrimmedCurve
std::pair<gp_Vec, gp_Vec> get_start_end_tangents(const Handle(Geom_TrimmedCurve) & curve);

gp_Vec get_end_tangent(const Handle(Geom_TrimmedCurve) & curve);
std::pair<gp_Vec, gp_Pnt> get_out_dir_and_end_pt(const Handle(Geom_TrimmedCurve) & curve);

std::pair<gp_Pnt, gp_Pnt>     get_edge_endpoints(const TopoDS_Edge& edge);
std::pair<gp_Pnt2d, gp_Pnt2d> get_edge_endpoints(const gp_Pln& pln, const TopoDS_Edge& edge);

enum class Plane_side
{
  Front,  // Positive side (normal direction)
  Back,   // Negative side (opposite normal)
  On      // On the plane
};

Plane_side side_of_plane(const gp_Pln& plane, const gp_Pnt& point);

// Function to get gp_Pln from a TopoDS_Face
std::optional<gp_Pln> plane_from_face(const TopoDS_Face& face);

bool planes_equal(const gp_Pln& plane1, const gp_Pln& plane2);

// Projects a vector onto a plane defined by its normal
gp_Vec project_onto_plane(const gp_Vec& v, const gp_Pln& pln);

gp_Pln xy_plane();

// Function to compute the center point between two gp_Pnt2d points
gp_Pnt2d center_point(const gp_Pnt2d& point1, const gp_Pnt2d& point2);

// Function to compute the normalized direction between two gp_Pnt2d points
gp_Dir2d get_unit_dir(const gp_Pnt2d& point1, const gp_Pnt2d& point2);

inline glm::dvec2 to_glm(const gp_Dir2d& v)
{
  return {v.X(), v.Y()};
}

gp_Pnt2d get_midpoint(const gp_Pnt2d& p1, const gp_Pnt2d& p2);

gp_Pnt2d mirror_point(const gp_Pnt2d& p1, const gp_Pnt2d& p2, const gp_Pnt2d& point_to_mirror);

PrsDim_LengthDimension_ptr create_distance_annotation(const gp_Pnt& p1,
                                                      const gp_Pnt& p2,
                                                      const gp_Pln& pln);

PrsDim_LengthDimension_ptr create_distance_annotation(const gp_Pnt2d& p1,
                                                      const gp_Pnt2d& p2,
                                                      const gp_Pln&   pln);

const gp_Pnt& closest_to_camera(const V3d_View_ptr& view, const std::vector<gp_Pnt>& pnts);

// Function to compute the area of a face
double compute_face_area(const AIS_Shape_ptr& shp);

// Function to check if shape_a is contained within shape_b (both must be faces, holes are not considered)
bool is_face_contained(const TopoDS_Shape& shape_a, const TopoDS_Shape& shape_b);

// Boost related, usefull for debugging using
// https://marketplace.visualstudio.com/items?itemName=AdamWulkiewicz.GraphicalDebugging
// Function to convert a 3D point to 2D in the plane's coordinate system
boost_geom::point_2d to_boost(const gp_Pln& plane, const gp_Pnt& point_3d);

boost_geom::point_2d to_boost(const gp_Pnt2d& point);

// Convert a TopoDS_Shape to a boost_geom::polygon_2d
boost_geom::polygon_2d to_boost(const TopoDS_Shape& shape, const gp_Pln& pln2);

gp_Pnt get_shape_bbox_center(const TopoDS_Shape& shp);

// Checks if elements are unique using IsEqual
template <typename T, typename... Args>
bool unique(const T& first, Args... args);

// Checks if elements are equal using IsEqual
template <typename T, typename... Args>
bool equal(const T& first, Args... args);

// Define custom less-than operator for gp_Pnt2d using Precision::Confusion()
bool operator<(const gp_Pnt2d& lhs, const gp_Pnt2d& rhs);

// Add this function declaration to geom.h
gp_Pnt2d rotate_point(const gp_Pnt2d& origin, const gp_Pnt2d& point, double angle_degrees);

// Check if a ring is clockwise using the shoelace formula
bool is_clockwise(const boost_geom::ring_2d& ring);

// Sorts a vector of gp_Pnt by x, then y, then z
void sort_pnts(std::vector<gp_Pnt>& points);

// Convert degrees to radians
constexpr double to_radians(double degrees);

// Convert radians to degrees
constexpr double to_degrees(double radians);

#include "geom.inl"