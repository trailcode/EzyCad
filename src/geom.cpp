#include "geom.h"

#include <AIS_Shape.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepGProp.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GProp_GProps.hxx>
#include <Geom2dAPI_ProjectPointOnCurve.hxx>
#include <Geom2d_Line.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Plane.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <PrsDim_LengthDimension.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <V3d_View.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <numbers>

// Function to project a 3D point onto a plane and get its 2D (u, v) coordinates
gp_Pnt2d to_2d(const gp_Pln& plane, const gp_Pnt& point_3d)
{
  // TODO use: GeomAPI_ProjectPointOnSurface
  // Step 1: Project the 3D point onto the plane
  // Get the plane's normal direction
  gp_Dir normal = plane.Axis().Direction();

  // Get a point on the plane (e.g., its origin)
  gp_Pnt origin = plane.Location();

  // Vector from origin to the 3D point
  gp_Vec vec_to_point(origin, point_3d);

  // Distance from point to plane along normal (signed distance)
  Standard_Real distance = vec_to_point.Dot(normal);

  // Projected point = 3D point - (distance * normal)
  gp_Vec normal_vec(normal);
  normal_vec.Multiply(distance);
  gp_Pnt projected_point = point_3d.Translated(-normal_vec);

  // Step 2: Convert projected point to 2D (u, v) coordinates
  // Get the plane's local coordinate system
  gp_Ax3 plane_axes = plane.Position();
  gp_Dir x_dir      = plane_axes.XDirection();
  gp_Dir y_dir      = plane_axes.YDirection();

  // Vector from plane origin to projected point
  gp_Vec vec_projected(origin, projected_point);

  // Compute u, v as projections onto X and Y directions
  Standard_Real u = vec_projected.Dot(x_dir);
  Standard_Real v = vec_projected.Dot(y_dir);

  return gp_Pnt2d(u, v);
}

// Convert 2D (u, v) point on gp_Pln to 3D point
gp_Pnt to_3d(const gp_Pln& plane, const gp_Pnt2d& point_2d)
{
  // Get the plane's coordinate system
  gp_Ax3 axes   = plane.Position();
  gp_Pnt origin = axes.Location();    // Plane origin
  gp_Dir x_dir  = axes.XDirection();  // u-axis
  gp_Dir y_dir  = axes.YDirection();  // v-axis

  // Compute 3D point: Origin + u * XDir + v * YDir
  gp_Vec u_vec(x_dir);
  u_vec.Multiply(point_2d.X());
  gp_Vec v_vec(y_dir);
  v_vec.Multiply(point_2d.Y());

  return origin.Translated(u_vec + v_vec);
}

gp_Pnt2d to_pnt2d(const boost_geom::point_2d& pt)
{
  return gp_Pnt2d(pt.x(), pt.y());
}

// Function to create a wire box centered on a point on a plane, returning a TopoDS_Wire
TopoDS_Wire create_wire_box(const gp_Pln& plane, const gp_Pnt& center, double width, double height)
{
  // Get the plane's coordinate system
  gp_Ax3 axes  = plane.Position();
  gp_Dir x_dir = axes.XDirection();  // Width along plane's X-axis
  gp_Dir y_dir = axes.YDirection();  // Height along plane's Y-axis

  // Half-width and half-height for centering
  double half_width  = width / 2.0;
  double half_height = height / 2.0;

  // Define vectors for offsets
  gp_Vec x_vec_plus(x_dir);
  x_vec_plus.Multiply(half_width);
  gp_Vec x_vec_minus(x_dir);
  x_vec_minus.Multiply(-half_width);
  gp_Vec y_vec_plus(y_dir);
  y_vec_plus.Multiply(half_height);
  gp_Vec y_vec_minus(y_dir);
  y_vec_minus.Multiply(-half_height);

  // Define the 4 corner points of the box, centered on 'center'
  gp_Pnt p1 = center.Translated(x_vec_minus + y_vec_minus);  // Bottom-left
  gp_Pnt p2 = center.Translated(x_vec_plus + y_vec_minus);   // Bottom-right
  gp_Pnt p3 = center.Translated(x_vec_plus + y_vec_plus);    // Top-right
  gp_Pnt p4 = center.Translated(x_vec_minus + y_vec_plus);   // Top-left

  // Create the 4 edges of the box
  BRepBuilderAPI_MakeEdge edge1(p1, p2);  // Bottom
  BRepBuilderAPI_MakeEdge edge2(p2, p3);  // Right
  BRepBuilderAPI_MakeEdge edge3(p3, p4);  // Top
  BRepBuilderAPI_MakeEdge edge4(p4, p1);  // Left

  // Assemble edges into a wire
  BRepBuilderAPI_MakeWire wire_maker;
  wire_maker.Add(edge1.Edge());
  wire_maker.Add(edge2.Edge());
  wire_maker.Add(edge3.Edge());
  wire_maker.Add(edge4.Edge());

  if (!wire_maker.IsDone())
  {
    std::cerr << "Failed to create wire box\n";
    return TopoDS_Wire();  // Return empty wire on failure
  }

  return wire_maker.Wire();
}

TopoDS_Wire make_square_wire(const gp_Pln&   pln,
                             const gp_Pnt2d& center,
                             const gp_Pnt2d& edge_midpoint)
{
  std::array<gp_Pnt2d, 4> corners = square_corners(center, edge_midpoint);

  // Convert to 3D
  gp_Pnt p1 = to_3d(pln, corners[0]);
  gp_Pnt p2 = to_3d(pln, corners[1]);
  gp_Pnt p3 = to_3d(pln, corners[2]);
  gp_Pnt p4 = to_3d(pln, corners[3]);

  // Create edges and wire
  BRepBuilderAPI_MakeWire wire_maker;
  wire_maker.Add(BRepBuilderAPI_MakeEdge(p1, p2).Edge());
  wire_maker.Add(BRepBuilderAPI_MakeEdge(p2, p3).Edge());
  wire_maker.Add(BRepBuilderAPI_MakeEdge(p3, p4).Edge());
  wire_maker.Add(BRepBuilderAPI_MakeEdge(p4, p1).Edge());

  return wire_maker.Wire();
}

std::array<gp_Pnt2d, 4> square_corners(const gp_Pnt2d& center, const gp_Pnt2d& edge_midpoint)
{
  DO_ASSERT(unique(center, edge_midpoint));

  std::array<gp_Pnt2d, 4> ret;

  // Compute side length from center to edge midpoint
  gp_Vec2d      to_midpoint(edge_midpoint.X() - center.X(),
                            edge_midpoint.Y() - center.Y());
  Standard_Real half_side = to_midpoint.Magnitude();

  // Direction to edge midpoint
  gp_Vec2d edge_dir = to_midpoint.Normalized();

  // Perpendicular direction (90 degrees)
  gp_Vec2d perp_dir = edge_dir.Rotated(std::numbers::pi / 2.0);

  // Compute 2D vertices
  ret[0] = gp_Pnt2d(center.X() + half_side * (edge_dir.X() + perp_dir.X()),
                    center.Y() + half_side * (edge_dir.Y() + perp_dir.Y()));
  ret[1] = gp_Pnt2d(center.X() + half_side * (edge_dir.X() - perp_dir.X()),
                    center.Y() + half_side * (edge_dir.Y() - perp_dir.Y()));
  ret[2] = gp_Pnt2d(center.X() - half_side * (edge_dir.X() + perp_dir.X()),
                    center.Y() - half_side * (edge_dir.Y() + perp_dir.Y()));
  ret[3] = gp_Pnt2d(center.X() - half_side * (edge_dir.X() - perp_dir.X()),
                    center.Y() - half_side * (edge_dir.Y() - perp_dir.Y()));

  return ret;
}

std::array<gp_Pnt2d, 4> xy_stencil_pnts(const gp_Pnt2d& center, const gp_Pnt2d& edge_midpoint)
{
  DO_ASSERT(unique(center, edge_midpoint));

  std::array<gp_Pnt2d, 4> ret;

  gp_Vec2d edge_dir(edge_midpoint.X() - center.X(),
                    edge_midpoint.Y() - center.Y());

  // Perpendicular direction (90 degrees)
  gp_Vec2d perp_dir = edge_dir.Rotated(std::numbers::pi / 2.0);

  ret[0] = gp_Pnt2d(center.XY() - edge_dir.XY());
  ret[1] = gp_Pnt2d(center.XY() + edge_dir.XY());
  ret[2] = gp_Pnt2d(center.XY() - perp_dir.XY());
  ret[3] = gp_Pnt2d(center.XY() + perp_dir.XY());

  return ret;
}

TopoDS_Wire make_circle_wire(const gp_Pln&   pln,
                             const gp_Pnt2d& center,
                             const gp_Pnt2d& edge_point)
{
  // Compute radius
  gp_Vec2d      to_edge(edge_point.X() - center.X(),
                        edge_point.Y() - center.Y());
  Standard_Real radius = to_edge.Magnitude();

  // Get 3D center and plane's normal
  gp_Pnt center_3d = to_3d(pln, center);
  gp_Dir normal    = pln.Axis().Direction();

  // Create circle geometry
  gp_Ax2 circle_axis(center_3d, normal);
  Handle(Geom_Circle) circle = new Geom_Circle(circle_axis, radius);

  // Make edge and wire
  BRepBuilderAPI_MakeEdge edge_maker(circle, 0.0, 2.0 * std::numbers::pi);
  BRepBuilderAPI_MakeWire wire_maker(edge_maker.Edge());

  return wire_maker.Wire();
}

Slot_pnts get_slot_points(const gp_Pnt2d& pt_a,
                          const gp_Pnt2d& pt_b,
                          const gp_Pnt2d& pt_c)
{
  Slot_pnts ret;

  // Compute radius
  gp_Vec2d to_c(pt_c.X() - pt_b.X(), pt_c.Y() - pt_b.Y());
  double   radius = to_c.Magnitude();

  // Direction from pt_a to pt_b
  gp_Vec2d a_to_b(pt_b.X() - pt_a.X(), pt_b.Y() - pt_a.Y());
  gp_Vec2d dir = a_to_b.Normalized();

  // Perpendicular direction (90 degrees counterclockwise)
  const double ninety_degrees_radians = std::numbers::pi / 2.0;
  gp_Vec2d     perp_dir               = dir.Rotated(ninety_degrees_radians);

  // Arc at pt_a: from +perp_dir (top) to -perp_dir (bottom)
  ret.a_top_2d    = gp_Pnt2d(pt_a.X() + radius * perp_dir.X(),
                             pt_a.Y() + radius * perp_dir.Y());
  ret.a_mid_2d    = gp_Pnt2d(pt_a.X() - radius * dir.X(),
                             pt_a.Y() - radius * dir.Y());
  ret.a_bottom_2d = gp_Pnt2d(pt_a.X() - radius * perp_dir.X(),
                             pt_a.Y() - radius * perp_dir.Y());

  // Arc at pt_b: from -perp_dir (bottom) to +perp_dir (top)
  ret.b_bottom_2d = gp_Pnt2d(pt_b.X() - radius * perp_dir.X(),
                             pt_b.Y() - radius * perp_dir.Y());
  ret.b_mid_2d    = gp_Pnt2d(pt_b.X() + radius * dir.X(),
                             pt_b.Y() + radius * dir.Y());
  ret.b_top_2d    = gp_Pnt2d(pt_b.X() + radius * perp_dir.X(),
                             pt_b.Y() + radius * perp_dir.Y());

  return ret;
}

TopoDS_Wire make_slot_wire(const gp_Pln&   plane,
                           const gp_Pnt2d& pt_a,
                           const gp_Pnt2d& pt_b,
                           const gp_Pnt2d& pt_c)
{
  Slot_pnts pnts = get_slot_points(pt_a, pt_b, pt_c);

  // Get 3D points
  gp_Pnt a_3d        = to_3d(plane, pt_a);
  gp_Pnt b_3d        = to_3d(plane, pt_b);
  gp_Pnt a_top_3d    = to_3d(plane, pnts.a_top_2d);
  gp_Pnt a_mid_3d    = to_3d(plane, pnts.a_mid_2d);
  gp_Pnt a_bottom_3d = to_3d(plane, pnts.a_bottom_2d);
  gp_Pnt b_bottom_3d = to_3d(plane, pnts.b_bottom_2d);
  gp_Pnt b_mid_3d    = to_3d(plane, pnts.b_mid_2d);
  gp_Pnt b_top_3d    = to_3d(plane, pnts.b_top_2d);

  GC_MakeArcOfCircle    arc_a_maker(a_top_3d, a_mid_3d, a_bottom_3d);
  GC_MakeArcOfCircle    arc_b_maker(b_bottom_3d, b_mid_3d, b_top_3d);
  Geom_TrimmedCurve_ptr arc_a = arc_a_maker.Value();
  Geom_TrimmedCurve_ptr arc_b = arc_b_maker.Value();

  // Straight segments
  BRepBuilderAPI_MakeEdge line_1(a_bottom_3d, b_bottom_3d);  // Bottom line
  BRepBuilderAPI_MakeEdge line_2(b_top_3d, a_top_3d);        // Top line

  // Build wire
  BRepBuilderAPI_MakeWire wire_maker;
  wire_maker.Add(BRepBuilderAPI_MakeEdge(arc_a).Edge());
  wire_maker.Add(line_1.Edge());
  wire_maker.Add(BRepBuilderAPI_MakeEdge(arc_b).Edge());
  wire_maker.Add(line_2.Edge());

  return wire_maker.Wire();
}

// Function to get the directional vectors at the start and end of a Geom_TrimmedCurve
std::pair<gp_Vec, gp_Vec> get_start_end_tangents(const Handle(Geom_TrimmedCurve) & curve)
{
  // Get the start and end parameters
  Standard_Real u_start = curve->FirstParameter();
  Standard_Real u_end   = curve->LastParameter();

  // Evaluate the curve at start and end points to get points and tangents
  gp_Pnt p_start, p_end;
  gp_Vec tangent_start, tangent_end;

  // D1 gives point (P) and first derivative (V1)
  curve->D1(u_start, p_start, tangent_start);
  curve->D1(u_end, p_end, tangent_end);

  // Normalize the tangent vectors to get unit directions
  DO_ASSERT_MSG(tangent_start.Magnitude() > 1e-10, "Tangent start vector too small!");
  DO_ASSERT_MSG(tangent_end.Magnitude() > 1e-10, "Tangent start vector too small!");

  tangent_start.Normalize();
  tangent_end.Normalize();

  return {tangent_start, tangent_end};
}

gp_Vec get_end_tangent(const Handle(Geom_TrimmedCurve) & curve)
{
  Standard_Real u_end = curve->LastParameter();
  gp_Vec        tangent_end;
  gp_Pnt        p_end;

  // D1 gives point (P) and first derivative (V1)
  curve->D1(u_end, p_end, tangent_end);

  // Normalize the tangent vector to get unit directions
  DO_ASSERT_MSG(tangent_end.Magnitude() > 1e-10, "Tangent start vector too small!");
  tangent_end.Normalize();

  return tangent_end;
}

std::pair<gp_Vec, gp_Pnt> get_out_dir_and_end_pt(const Handle(Geom_TrimmedCurve) & curve)
{
  Standard_Real u_end = curve->LastParameter();
  gp_Vec        out_dir;
  gp_Vec        unused;
  gp_Pnt        p_end;

  // D1 gives point (P) and first second (V1)
  curve->D2(u_end, p_end, unused, out_dir);

  // Normalize the out direction vector to get unit directions
  DO_ASSERT_MSG(out_dir.Magnitude() > 1e-10, "Start vector too small!");
  out_dir.Normalize();

  return {out_dir, p_end};
}

std::pair<gp_Pnt, gp_Pnt> get_edge_endpoints(const TopoDS_Edge& edge)
{
  // Extract start and end vertices
  TopoDS_Vertex start_vertex, end_vertex;
  TopExp::Vertices(edge, start_vertex, end_vertex);

  // Check for null vertices
  DO_ASSERT(!start_vertex.IsNull() && !end_vertex.IsNull());

  // Convert vertices to geometric points
  gp_Pnt start_point = BRep_Tool::Pnt(start_vertex);
  gp_Pnt end_point   = BRep_Tool::Pnt(end_vertex);

  // Return the pair of points
  return std::make_pair(start_point, end_point);
}

std::pair<gp_Pnt2d, gp_Pnt2d> get_edge_endpoints(const gp_Pln& pln, const TopoDS_Edge& edge)
{
  const auto [pt_a, pt_b] = get_edge_endpoints(edge);
  return {to_2d(pln, pt_a), to_2d(pln, pt_b)};
}

Plane_side side_of_plane(const gp_Pln& plane, const gp_Pnt& point)
{
  // Get the plane's origin and normal
  gp_Pnt plane_origin = plane.Location();
  gp_Dir plane_normal = plane.Axis().Direction();

  // Vector from plane origin to the point
  gp_Vec origin_to_point(plane_origin, point);

  // Compute the signed distance (dot product with normal)
  double signed_distance = origin_to_point.Dot(plane_normal);

  // Determine the side with tolerance
  if (signed_distance > 1e-6)
    return Plane_side::Front;  // Point is on the front side

  else if (signed_distance < -1e-6)
    return Plane_side::Back;  // Point is on the back side

  return Plane_side::On;  // Point is on the plane
}

// Function to get gp_Pln from a TopoDS_Face
std::optional<gp_Pln> plane_from_face(const TopoDS_Face& face)
{
  if (face.IsNull())
    return std::nullopt;

  // Get the underlying surface
  Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
  if (surface.IsNull())
    return std::nullopt;

  // Check if the surface is a plane and cast it
  Handle(Geom_Plane) plane_surface = Handle(Geom_Plane)::DownCast(surface);
  if (plane_surface.IsNull())
    return std::nullopt;

  // Return the gp_Pln
  return plane_surface->Pln();
}

bool planes_equal(const gp_Pln& plane1, const gp_Pln& plane2)
{
  // Get the position and orientation of both planes
  gp_Ax3 pos1 = plane1.Position();
  gp_Ax3 pos2 = plane2.Position();

  // Check if the origins are the same (within precision)
  if (!pos1.Location().IsEqual(pos2.Location(), Precision::Confusion()))
    return false;

  // Check if the normal directions are parallel (within angular precision)
  if (!pos1.Direction().IsParallel(pos2.Direction(), Precision::Angular()))
    return false;

  // Check if the X directions are parallel (within angular precision)
  if (!pos1.XDirection().IsParallel(pos2.XDirection(), Precision::Angular()))
    return false;

  return true;
}

// Projects a vector onto a plane defined by its normal
gp_Vec project_onto_plane(const gp_Vec& v, const gp_Pln& pln)
{
  gp_Dir normal = pln.Axis().Direction();
  gp_Vec n(normal);
  return v - n * (v.Dot(n));
}

gp_Pln xy_plane()
{
  gp_Ax3 xy_system = gp::XOY();                                // Predefined XY coordinate system
  return gp_Pln(xy_system.Location(), xy_system.Direction());  // XY plane
}

// Function to compute the center point between two gp_Pnt2d points
gp_Pnt2d center_point(const gp_Pnt2d& point1, const gp_Pnt2d& point2)
{
  // Calculate midpoint coordinates
  Standard_Real center_x = (point1.X() + point2.X()) / 2.0;
  Standard_Real center_y = (point1.Y() + point2.Y()) / 2.0;

  // Return the new center point
  return gp_Pnt2d(center_x, center_y);
}

// Function to compute the normalized direction between two gp_Pnt2d points
gp_Dir2d get_unit_dir(const gp_Pnt2d& point1, const gp_Pnt2d& point2)
{
  // Calculate the vector from point1 to point2
  gp_Vec2d direction_vector(point1, point2);

  // Check if the vector is zero-length (points are identical)
  DO_ASSERT_MSG(direction_vector.Magnitude() > Precision::Confusion(),
                "Error: Points are coincident");

  // Normalize the vector and return as gp_Dir2d
  return gp_Dir2d(direction_vector.Normalized());
}

gp_Pnt2d get_midpoint(const gp_Pnt2d& p1, const gp_Pnt2d& p2)
{
  Standard_Real x = (p1.X() + p2.X()) * 0.5;
  Standard_Real y = (p1.Y() + p2.Y()) * 0.5;
  return gp_Pnt2d(x, y);
}

gp_Pnt2d mirror_point(const gp_Pnt2d& p1, const gp_Pnt2d& p2, const gp_Pnt2d& point_to_mirror)
{
  // Check if p1 and p2 are the same (invalid line)
  DO_ASSERT(unique(p1, p2));

  // Define the line direction and create a Geom2d_Line
  gp_Vec2d line_vec(p1, p2);
  gp_Dir2d line_dir(line_vec);

  Handle(Geom2d_Curve) line_ptr = new Geom2d_Line(p1, line_dir);
  // Project the point onto the line
  Geom2dAPI_ProjectPointOnCurve projector(point_to_mirror, line_ptr);
  DO_ASSERT(projector.NbPoints() != 0);

  gp_Pnt2d projection = projector.NearestPoint();

  // Compute the vector from projection to the point to mirror
  gp_Vec2d vec_to_point(projection, point_to_mirror);

  // Mirror the point: move twice the vector distance in the opposite direction
  gp_Pnt2d mirrored_point = projection.Translated(-vec_to_point);

  return mirrored_point;
}

PrsDim_LengthDimension_ptr create_distance_annotation(const gp_Pnt& p1,
                                                      const gp_Pnt& p2,
                                                      const gp_Pln& pln)
{
  // Check if points are too close (invalid for dimension)
  DO_ASSERT(unique(p1, p2));

  // Convert gp_Pnt to TopoDS_Vertex
  TopoDS_Vertex vertex_1 = BRepBuilderAPI_MakeVertex(p1);
  TopoDS_Vertex vertex_2 = BRepBuilderAPI_MakeVertex(p2);

  // Create and return the length dimension annotation
  return new PrsDim_LengthDimension(vertex_1, vertex_2, pln);
}

PrsDim_LengthDimension_ptr create_distance_annotation(const gp_Pnt2d& p1,
                                                      const gp_Pnt2d& p2,
                                                      const gp_Pln&   pln)
{
  gp_Pnt point_1 = to_3d(pln, p1);
  gp_Pnt point_2 = to_3d(pln, p2);

  return create_distance_annotation(point_1, point_2, pln);
}

const gp_Pnt& closest_to_camera(const V3d_View_ptr& view, const std::vector<gp_Pnt>& pnts)
{
  DO_ASSERT(pnts.size());
  size_t        best_idx;
  Standard_Real min_distance = std::numeric_limits<Standard_Real>::max();

  // Get the camera's eye position (world coordinates)
  gp_Pnt camera_pos = view->Camera()->Eye();

  for (size_t idx = 0, num = pnts.size(); idx < num; ++idx)
  {
    // Compute Euclidean distance to camera
    Standard_Real distance = pnts[idx].Distance(camera_pos);

    // Update closest point if this distance is smaller
    if (distance < min_distance)
    {
      min_distance = distance;
      best_idx     = idx;
    }
  }

  return pnts[best_idx];
}

// Function to compute the area of a face
double compute_face_area(const AIS_Shape_ptr& shp)
{
  // Ensure the shape is a face
  const TopoDS_Shape& shape = shp->Shape();
  DO_ASSERT_MSG(shape.ShapeType() == TopAbs_FACE, "Shape is not a face.");

  // Cast to TopoDS_Face
  TopoDS_Face face = TopoDS::Face(shape);

  // Compute surface properties
  GProp_GProps props;
  BRepGProp::SurfaceProperties(face, props);

  // Return the area (mass in surface properties)
  return props.Mass();
}

// Function to check if shape_a is contained within shape_b (both must be faces, holes are not considered)
bool is_face_contained(const TopoDS_Shape& shape_a, const TopoDS_Shape& shape_b)
{
  // Verify both shapes are faces
  if (shape_a.ShapeType() != TopAbs_FACE || shape_b.ShapeType() != TopAbs_FACE)
  {
    throw std::runtime_error("Both shapes must be faces.");
  }

  // Cast to TopoDS_Face
  const TopoDS_Face& face_a = TopoDS::Face(shape_a);
  const TopoDS_Face& face_b = TopoDS::Face(shape_b);

  // Check surface compatibility (both planar and coplanar)
  Handle(Geom_Surface) surface_a = BRep_Tool::Surface(face_a);
  Handle(Geom_Surface) surface_b = BRep_Tool::Surface(face_b);

  Handle(Geom_Plane) plane_a = Handle(Geom_Plane)::DownCast(surface_a);
  Handle(Geom_Plane) plane_b = Handle(Geom_Plane)::DownCast(surface_b);

  if (plane_a.IsNull() || plane_b.IsNull())
  {
    throw std::runtime_error("Both faces must be planar.");
  }

  gp_Pln pln_a = plane_a->Pln();
  gp_Pln pln_b = plane_b->Pln();

  if (!pln_a.Position().Direction().IsParallel(pln_b.Position().Direction(), Precision::Angular()) ||
      Abs(pln_a.Distance(pln_b.Location())) > Precision::Confusion())
  {
    return Standard_False;  // Not coplanar
  }

  // Check if face_a's outer wire is contained within face_b
  TopoDS_Wire      wire_a = BRepTools::OuterWire(face_a);
  TopExp_Explorer  vertex_explorer(wire_a, TopAbs_VERTEX);
  Standard_Boolean all_inside = Standard_True;

  while (vertex_explorer.More() && all_inside)
  {
    TopoDS_Vertex vertex = TopoDS::Vertex(vertex_explorer.Current());
    gp_Pnt        point  = BRep_Tool::Pnt(vertex);

    BRepClass_FaceClassifier classifier(face_b, point, Precision::Confusion());
    TopAbs_State             state = classifier.State();

    if (state != TopAbs_IN)
      all_inside = Standard_False;

    vertex_explorer.Next();
  }

  return all_inside;
}

// Function to convert a 3D point to 2D in the plane's coordinate system
boost_geom::point_2d to_boost(const gp_Pln& plane, const gp_Pnt& point_3d)
{
  gp_Pnt2d pt = to_2d(plane, point_3d);
  return {pt.X(), pt.Y()};
}

boost_geom::point_2d to_boost(const gp_Pnt2d& pt)
{
  return {pt.X(), pt.Y()};
}

// Convert a TopoDS_Shape to a boost_geom::polygon_2d
boost_geom::polygon_2d to_boost(const TopoDS_Shape& shape, const gp_Pln& pln2)
{
  // Check if the shape is a face
  DO_ASSERT_MSG(shape.ShapeType() == TopAbs_FACE, "Shape must be a face");

  // Cast to TopoDS_Face
  const TopoDS_Face& face = TopoDS::Face(shape);

  // Get the surface and verify it's a plane
  Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
  Handle(Geom_Plane) plane     = Handle(Geom_Plane)::DownCast(surface);
  DO_ASSERT_MSG(!plane.IsNull(), "Face surface must be planar.");

  // Get the plane's coordinate system
  // gp_Pln pln = plane->Pln(); // Centers poly using the origin of the shape plane.
  const gp_Pln& pln = pln2;

  // Initialize the Boost.Geometry polygon
  boost_geom::polygon_2d polygon;

  // Get the outer wire
  TopoDS_Wire outer_wire = BRepTools::OuterWire(face);

  auto add_pt_unique = [](boost_geom::ring_2d& out, const gp_Pnt2d& pt)
  {
    if (out.empty())
    {
      out.push_back({pt.X(), pt.Y()});
      return;
    }

    // Get the last point in the ring
    const auto& last_pt = out.back();
    gp_Pnt2d    last_gp_pt(last_pt.x(), last_pt.y());

    // Only add if the point is different from the last one
    if (!last_gp_pt.IsEqual(pt, Precision::Confusion()))
    {
      out.push_back({pt.X(), pt.Y()});
    }
  };

  auto get_wire_verts = [&](const TopoDS_Wire& wire, boost_geom::ring_2d& out)
  {
    TopExp_Explorer edge_explorer(wire, TopAbs_EDGE);
    while (edge_explorer.More())
    {
      const TopoDS_Edge& edge = TopoDS::Edge(edge_explorer.Current());
      BRepAdaptor_Curve  curve(edge);
      GeomAbs_CurveType  curveType = curve.GetType();
      switch (curveType)
      {
        case GeomAbs_CurveType::GeomAbs_Line:
        {
          TopExp_Explorer vertexExplorer(edge, TopAbs_VERTEX);
          while (vertexExplorer.More())
          {
            const TopoDS_Vertex& vertex = TopoDS::Vertex(vertexExplorer.Current());
            gp_Pnt2d             pt     = to_2d(pln, BRep_Tool::Pnt(vertex));
            add_pt_unique(out, pt);
            vertexExplorer.Next();
          }
          break;
        }
        case GeomAbs_CurveType::GeomAbs_Circle:
        {
          // Get the parameter range of the curve
          double       u_start = curve.FirstParameter();
          double       u_end   = curve.LastParameter();
          const size_t num_pts = 25;
          double       step    = (u_end - u_start) / (num_pts - 1);
          for (size_t i = 0; i < num_pts; ++i)
          {
            gp_Pnt2d pt = to_2d(pln, curve.Value(u_start + i * step));
            add_pt_unique(out, pt);
          }
          break;
        }
        default:
          DO_ASSERT(false);  // Implement!
      }
      edge_explorer.Next();
    }
    if (wire.Orientation() == TopAbs_FORWARD)
      std::reverse(out.begin(), out.end());

    DO_ASSERT_MSG(to_pnt2d(out.front()).IsEqual(to_pnt2d(out.back()), Precision::Confusion()),
                  "Ring not closed!");

    out.pop_back();

    // Find the leftmost point (lowest x, breaking ties with y)
    auto leftmost_it = std::min_element(out.begin(), out.end(),
                                        [](const boost_geom::point_2d& a, const boost_geom::point_2d& b)
                                        {
                                          if (std::abs(a.x() - b.x()) > Precision::Confusion())
                                            return a.x() < b.x();

                                          return a.y() < b.y();
                                        });

    // Rotate the points so the leftmost point is first
    // Ensures consistency for comparisons
    std::rotate(out.begin(), leftmost_it, out.end());
    out.push_back(out.front());
  };

  get_wire_verts(outer_wire, polygon.outer());

  auto check_ring = [](boost_geom::ring_2d& ring)
  {
    DO_ASSERT_MSG(ring.size() > 3, "Not enough points!");

    // Boost requires points to be exactly the same.
    ring.front() = ring.back();
  };

  check_ring(polygon.outer());

  // Convert inner wires (holes)
  TopExp_Explorer wire_explorer(face, TopAbs_WIRE);
  while (wire_explorer.More())
  {
    TopoDS_Wire wire = TopoDS::Wire(wire_explorer.Current());
    if (!wire.IsSame(outer_wire))  // TODO Is this expensive? Better way?
    {
      boost_geom::ring_2d inner;
      get_wire_verts(wire, inner);
      check_ring(inner);
      polygon.inners().emplace_back(inner);
    }
    wire_explorer.Next();
  }

  // Validate the polygon
  if (!boost::geometry::is_valid(polygon))
  {
    // throw std::runtime_error("Invalid Boost.Geometry polygon created.");
  }

  return polygon;
}

gp_Pnt get_shape_bbox_center(const TopoDS_Shape& shp)
{
  Bnd_Box bbox;
  BRepBndLib::Add(shp, bbox);
  Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
  bbox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
  return gp_Pnt((xMin + xMax) / 2.0, (yMin + yMax) / 2.0, (zMin + zMax) / 2.0);
}

// Define custom less-than operator for gp_Pnt2d using Precision::Confusion()
bool operator<(const gp_Pnt2d& lhs, const gp_Pnt2d& rhs)
{
  double tolerance = Precision::Confusion();

  // Check if points are equal within tolerance
  if (lhs.Distance(rhs) <= tolerance)
  {
    return false;  // Equal points are not less than each other
  }

  // Lexicographical ordering: compare X first, then Y if X is equal within tolerance
  if (std::abs(lhs.X() - rhs.X()) > tolerance)
  {
    return lhs.X() < rhs.X();
  }
  return lhs.Y() < rhs.Y();
}

gp_Pnt2d rotate_point(const gp_Pnt2d& origin, const gp_Pnt2d& point, double angle_degrees)
{
  // Convert angle from degrees to radians
  const double angle_rad = angle_degrees * std::numbers::pi / 180.0;

  // Calculate the vector from origin to point
  const double dx = point.X() - origin.X();
  const double dy = point.Y() - origin.Y();

  // Calculate cos and sin of the angle
  const double cos_angle = cos(angle_rad);
  const double sin_angle = sin(angle_rad);

  // Apply rotation matrix:
  // [cos(θ) -sin(θ)] [x]
  // [sin(θ)  cos(θ)] [y]
  const double rotated_x = dx * cos_angle - dy * sin_angle;
  const double rotated_y = dx * sin_angle + dy * cos_angle;

  // Add back the origin offset
  return gp_Pnt2d(origin.X() + rotated_x, origin.Y() + rotated_y);
}

bool is_clockwise(const boost_geom::ring_2d& ring)
{
  double sum = 0.0;
  for (size_t i = 0; i < ring.size() - 1; ++i)
  {
    sum += (ring[i + 1].x() - ring[i].x()) * (ring[i + 1].y() + ring[i].y());
  }
  return sum > 0.0;
}

// Sorts a vector of gp_Pnt by x, then y, then z
void sort_pnts(std::vector<gp_Pnt>& points)
{
  constexpr double tol = Precision::Confusion();
  std::sort(points.begin(), points.end(),
            [tol](const gp_Pnt& a, const gp_Pnt& b)
            {
              if (std::abs(a.X() - b.X()) > tol)
                return a.X() < b.X();
              if (std::abs(a.Y() - b.Y()) > tol)
                return a.Y() < b.Y();
              return a.Z() < b.Z();
            });
}