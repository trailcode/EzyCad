#pragma once

#include <Prs3d_DimensionTextHorizontalPosition.hxx>
#include <Standard_Handle.hxx>
#include <array>
#include <glm/glm.hpp>
#include <gp_Dir2d.hxx>
#include <gp_Pnt.hxx>
#include <numbers> // For Pi
#include <optional>
#include <vector>

#include "utl_dbg.h"
#include "utl_types.h"

class gp_Pln;
class gp_Pnt2d;
class gp_Vec;
class TopoDS_Edge;
class TopoDS_Wire;
class TopoDS_Face;
class TopoDS_Shape;
class Geom_TrimmedCurve;

#include "utl_geom_boost.inl"

gp_Pnt2d             to_pnt2d(const ezy_geom::point_2d& pt);
ezy_geom::point_2d   to_boost(const gp_Pln& plane, const gp_Pnt& point_3d);
ezy_geom::point_2d   to_boost(const gp_Pnt2d& point);
ezy_geom::polygon_2d to_boost(const TopoDS_Shape& shape, const gp_Pln& pln2);
ezy_geom::ring_2d    to_boost_ls(const TopoDS_Shape& shape, const gp_Pln& pln);
bool                 is_clockwise(const ezy_geom::ring_2d& ring);

// Simple WKT writers (used by tests and Graphical Debugging log output).
std::string to_wkt_string(const ezy_geom::ring_2d& ring);
std::string to_wkt_string(const ezy_geom::polygon_2d& poly);

// Function to project a 3D point onto a plane and get its 2D (u, v) coordinates
gp_Pnt2d to_2d(const gp_Pln& plane, const gp_Pnt& point_3d);

// Convert 2D (u, v) point on gp_Pln to 3D point
gp_Pnt to_3d(const gp_Pln& plane, const gp_Pnt2d& point_2d);

// Function to create a wire box centered on a point on a plane, returning a TopoDS_Wire
TopoDS_Wire create_wire_box(const gp_Pln& plane, const gp_Pnt& position, double width, double height);

/// Two perpendicular segments forming a + on \a plane, centered at \a center_3d, half-length \a half_arm (model units).
TopoDS_Shape create_plus_cross_shape(const gp_Pln& plane, const gp_Pnt& center_3d, double half_arm);

/// Sketch origin marker: + cross with a surrounding circle on \a plane (model units).
TopoDS_Shape create_origin_marker_shape(const gp_Pln& plane, const gp_Pnt& center_3d, double half_arm);

TopoDS_Wire make_square_wire(const gp_Pln& pln, const gp_Pnt2d& center, const gp_Pnt2d& edge_midpoint);

std::array<gp_Pnt2d, 4> square_corners(const gp_Pnt2d& center, const gp_Pnt2d& edge_midpoint);

std::array<gp_Pnt2d, 4> xy_stencil_pnts(const gp_Pnt2d& center, const gp_Pnt2d& edge_midpoint);

TopoDS_Wire make_circle_wire(const gp_Pln& pln, const gp_Pnt2d& center, const gp_Pnt2d& edge_point);

struct Slot_pnts
{
  gp_Pnt2d a_top_2d;
  gp_Pnt2d a_mid_2d;
  gp_Pnt2d a_bottom_2d;

  gp_Pnt2d b_bottom_2d;
  gp_Pnt2d b_mid_2d;
  gp_Pnt2d b_top_2d;
};

Slot_pnts get_slot_points(const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c);

TopoDS_Wire make_slot_wire(const gp_Pln& plane, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c);

// Function to get the directional vectors at the start and end of a Geom_TrimmedCurve
std::pair<gp_Vec, gp_Vec> get_start_end_tangents(const Handle(Geom_TrimmedCurve) & curve);

std::pair<gp_Pnt, gp_Pnt>     get_edge_endpoints(const TopoDS_Edge& edge);
std::pair<gp_Pnt2d, gp_Pnt2d> get_edge_endpoints(const gp_Pln& pln, const TopoDS_Edge& edge);

enum class Plane_side
{
  Front, // Positive side (normal direction)
  Back,  // Negative side (opposite normal)
  On     // On the plane
};

Plane_side side_of_plane(const gp_Pln& plane, const gp_Pnt& point);

// Function to get gp_Pln from a TopoDS_Face
std::optional<gp_Pln> plane_from_face(const TopoDS_Face& face);

bool planes_equal(const gp_Pln& plane1, const gp_Pln& plane2);

// Projects a vector onto a plane defined by its normal
gp_Vec project_onto_plane(const gp_Vec& v, const gp_Pln& pln);

gp_Pln xy_plane();

enum class Sketch_ref_plane
{
  XY,
  XZ,
  YZ,
};

/// Reference sketch plane through the origin, offset along the plane normal (\a offset in model units).
gp_Pln sketch_reference_plane(Sketch_ref_plane plane, double offset);

// Function to compute the center point between two gp_Pnt2d points
gp_Pnt2d center_point(const gp_Pnt2d& point1, const gp_Pnt2d& point2);

// Function to compute the normalized direction between two gp_Pnt2d points
gp_Dir2d get_unit_dir(const gp_Pnt2d& point1, const gp_Pnt2d& point2);

inline glm::dvec2 to_glm(const gp_Dir2d& v) { return {v.X(), v.Y()}; }

gp_Pnt2d get_midpoint(const gp_Pnt2d& p1, const gp_Pnt2d& p2);

gp_Pnt2d mirror_point(const gp_Pnt2d& p1, const gp_Pnt2d& p2, const gp_Pnt2d& point_to_mirror);

/// Global length-dimension display settings (GUI / `ezycad_settings.json` -> `gui.*`).
struct Length_dimension_style
{
  float line_width        = 1.0f;
  float arrow_size        = 6.0f;
  float color_rgb[3]      = {0.542373f, 0.542373f, 0.213732f};
  float text_height_scale = 1.0f;
  int   label_h           = 3;
  /// 0 standard, 1 sharp, 2 wide, 3 shaded 3D (see `edge_dim_arrow_style` in settings).
  int arrow_style = 0;
  /// 0 fit, 1 internal, 2 external (`Prs3d_DAO_*`).
  int arrow_orientation = 0;
  /// Label rendering mode (`gui.edge_dim_text_render_mode`): 0..5, default 5 (Z-layer Topmost).
  int text_render_mode = 5;
};

/// Maps edge length label index (0-3) to OCCT horizontal text placement.
Prs3d_DimensionTextHorizontalPosition edge_dim_text_h_pos_from_index(int idx);

/// Automatic flyout distance from edge length (before per-dimension override in Sketch List).
double length_dimension_auto_flyout(double edge_len);

/// Apply full dimension aspect (line, text, arrows, extensions). Call `Redisplay` after.
void apply_length_dimension_style(const PrsDim_LengthDimension_ptr& dim, const Length_dimension_style& style);

/// Sketch-list row hover: recolor and thicken the dimension line (call `Redisplay` after).
void apply_length_dimension_list_hover_style(const PrsDim_LengthDimension_ptr& dim, const float hover_rgb[3],
                                             double hover_line_width);

/// When `sketch_faces_for_flyout` is non-null and non-empty, edge dimensions offset to the side that is
/// void (not TopAbs_IN) relative to those faces - fixes concave / notch edges where the node centroid lies
/// on the wrong side. Otherwise `interior_ref` (e.g. node centroid) is used as a weaker heuristic.
PrsDim_LengthDimension_ptr create_distance_annotation(const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pln& pln,
                                                      const Length_dimension_style&   style,
                                                      const std::optional<gp_Pnt>&    interior_ref            = std::nullopt,
                                                      const std::vector<TopoDS_Face>* sketch_faces_for_flyout = nullptr);

PrsDim_LengthDimension_ptr create_distance_annotation(const gp_Pnt2d& p1, const gp_Pnt2d& p2, const gp_Pln& pln,
                                                      const Length_dimension_style&   style,
                                                      const std::optional<gp_Pnt>&    interior_ref            = std::nullopt,
                                                      const std::vector<TopoDS_Face>* sketch_faces_for_flyout = nullptr);

const gp_Pnt& closest_to_camera(const V3d_View_ptr& view, const std::vector<gp_Pnt>& pnts);

// Function to compute the area of a face
double compute_face_area(const AIS_Shape_ptr& shp);

// Function to check if shape_a is contained within shape_b (both must be faces, holes are not considered)
bool is_face_contained(const TopoDS_Shape& shape_a, const TopoDS_Shape& shape_b);

// Pure C++ geometry helpers (polygon from OCCT face, etc.), useful for debugging
// and for test validation of face winding/order/area.
ezy_geom::point_2d to_boost(const gp_Pln& plane, const gp_Pnt& point_3d);

ezy_geom::point_2d to_boost(const gp_Pnt2d& point);

// Convert a TopoDS_Shape to a ezy_geom::polygon_2d
ezy_geom::polygon_2d to_boost(const TopoDS_Shape& shape, const gp_Pln& pln2);

/// Edge as an open 2D polyline on \a pln (arc edges are densified).
ezy_geom::ring_2d to_boost_ls(const TopoDS_Shape& shape, const gp_Pln& pln);

gp_Pnt get_shape_bbox_center(const TopoDS_Shape& shp);

// Checks if elements are unique using IsEqual
template <typename T, typename... Args> bool unique(const T& first, Args... args);

// Checks if elements are equal using IsEqual
template <typename T, typename... Args> bool equal(const T& first, Args... args);

// Define custom less-than operator for gp_Pnt2d using Precision::Confusion()
bool operator<(const gp_Pnt2d& lhs, const gp_Pnt2d& rhs);

// Add this function declaration to geom.h
gp_Pnt2d rotate_point(const gp_Pnt2d& origin, const gp_Pnt2d& point, double angle_degrees);

// Check if a ring is clockwise using the shoelace formula
bool is_clockwise(const ezy_geom::ring_2d& ring);

// Sorts a vector of gp_Pnt by x, then y, then z
void sort_pnts(std::vector<gp_Pnt>& points);

// Convert degrees to radians
constexpr double to_radians(double degrees);

// Convert radians to degrees
constexpr double to_degrees(double radians);

TopoDS_Wire make_rectangle_wire(const gp_Pln& pln, const gp_Pnt2d& corner1, const gp_Pnt2d& corner2);

std::array<gp_Pnt2d, 4> rectangle_corners(const gp_Pnt2d& corner1, const gp_Pnt2d& corner2);
bool                    point_on_open_segment_2d(const gp_Pnt2d& p, const gp_Pnt2d& a, const gp_Pnt2d& b);

/// Controls whether segment endpoints are considered part of the segment
/// for intersection tests.
enum class Segment_inclusion
{
  Open,  // Intersection must be strictly interior to the segment (excludes endpoints)
  Closed // Intersection may occur at the endpoints (includes T-junctions and touches)
};

/// Returns the intersection point of the two line *segments* [a1-a2] and [b1-b2] if they intersect,
/// according to the given \a inclusion mode. Uses a tolerance for floating-point robustness.
/// Returns nullopt if the lines are (nearly) parallel/collinear (overlaps not supported) or if the
/// intersection point does not lie on the segments per the inclusion mode.
std::optional<gp_Pnt2d> segment_intersection_2d(const gp_Pnt2d& a1, const gp_Pnt2d& a2, const gp_Pnt2d& b1, const gp_Pnt2d& b2,
                                                Segment_inclusion inclusion = Segment_inclusion::Closed);

/// Adds \a p to the vector only if no existing point is within Precision::Confusion() distance.
/// Used to collect unique intersection points without duplicates.
void add_unique_point(std::vector<gp_Pnt2d>& points, const gp_Pnt2d& p);

/// If the shortest distance from \a p to segment `a-b` is <= \a max_perp_dist and the foot lies strictly
/// inside the segment (not near endpoints), returns that foot; otherwise nullopt.
std::optional<gp_Pnt2d> snap_foot_to_open_segment_interior_if_close(const gp_Pnt2d& p, const gp_Pnt2d& a, const gp_Pnt2d& b,
                                                                    double max_perp_dist);

/// True when \a p lies on the open interior of \a arc_edge (excluding endpoints).
[[nodiscard]] bool point_on_open_arc_interior_2d(const gp_Pnt2d& p, const TopoDS_Edge& arc_edge, const gp_Pln& pln);

/// Point at half the parametric length of \a arc_edge, projected to \a pln.
[[nodiscard]] gp_Pnt2d arc_curve_midpoint_2d(const TopoDS_Edge& arc_edge, const gp_Pln& pln);

/// Intersection points of segment [seg_a-seg_b] with \a arc_edge per \a inclusion.
[[nodiscard]] std::vector<gp_Pnt2d> segment_arc_intersections_2d(const gp_Pnt2d& seg_a, const gp_Pnt2d& seg_b,
                                                                 const TopoDS_Edge& arc_edge, const gp_Pln& pln,
                                                                 Segment_inclusion inclusion = Segment_inclusion::Closed);

/// Interior intersection points of two arc edges (excluding endpoint touches).
[[nodiscard]] std::vector<gp_Pnt2d> arc_arc_intersections_2d(const TopoDS_Edge& arc_a, const TopoDS_Edge& arc_b,
                                                             const gp_Pln& pln);

#include "utl_geom.inl"