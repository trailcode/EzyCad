#include <gtest/gtest.h>

#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <boost/geometry/io/wkt/wkt.hpp>
#include <numbers>

#include "geom.h"
#include "gui.h"
#include "occt_view.h"
#include "sketch.h"

namespace bg = boost::geometry;

// General templated helper for any Boost.Geometry geometry type
template <typename Geometry>
std::string to_wkt_string(const Geometry& geom)
{
  std::stringstream ss;
  ss << boost::geometry::wkt(geom);
  return ss.str();
}

class GUI_access
{
 public:
  static void       set_view(GUI& gui, std::unique_ptr<Occt_view>& view);
  static Occt_view& get_view(GUI& gui);
};

// Test fixture for Sketch tests
class Sketch_test : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    // TODO it is creating a view each time, create only one view and have a reset method.
    std::unique_ptr<Occt_view> view = std::make_unique<Occt_view>(s_gui);
    view->init_viewer();
    view->init_default();
    GUI_access::set_view(s_gui, view);
  }

  void TearDown() override
  {
    // view.reset();
  }

  Occt_view& view()
  {
    return GUI_access::get_view(s_gui);
  }

  GUI& gui()
  {
    return s_gui;
  }

  // Helper to add all permutations of edges (with both orientations) to a sketch and call a lambda
  template <typename Callback>
  void for_all_edge_permutations_(
      const std::vector<gp_Pnt2d>&            points,
      const std::vector<std::pair<int, int>>& edge_indices,
      const gp_Pln&                           plane,
      Callback&&                              callback);

  // Helper to add edges to a sketch from a vector of points and edge index pairs
  template <typename Callback>
  void add_edges_from_indices_(
      const std::vector<gp_Pnt2d>&            points,
      const std::vector<std::pair<int, int>>& edge_indices,
      const gp_Pln&                           plane,
      Callback&&                              callback);

  static GUI s_gui;
  // std::unique_ptr<Occt_view> view;
};

GUI Sketch_test::s_gui;

class Sketch_access
{
 public:
  static void                                    add_edge_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, bool add_dim_anno = false);
  static const std::vector<Sketch_face_shp_ptr>& get_faces(const Sketch& sketch);
  static void                                    update_faces_(Sketch& sketch);
  static void                                    add_arc_circle_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c);
  static void                                    get_originating_face_snp_pts_3d_(Sketch& sketch, std::vector<gp_Pnt>& out);
};

void Sketch_access::add_edge_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, bool add_dim_anno)
{
  sketch.add_edge_(pt_a, pt_b, add_dim_anno);
}

const std::vector<Sketch_face_shp_ptr>& Sketch_access::get_faces(const Sketch& sketch)
{
  return sketch.m_faces;
}

void Sketch_access::update_faces_(Sketch& sketch)
{
  sketch.update_faces_();
}

void Sketch_access::add_arc_circle_(Sketch& sketch, const gp_Pnt2d& pt_a, const gp_Pnt2d& pt_b, const gp_Pnt2d& pt_c)
{
  sketch.add_arc_circle_(pt_a, pt_b, pt_c);
}

void GUI_access::set_view(GUI& gui, std::unique_ptr<Occt_view>& view)
{
  gui.m_view = std::move(view);
}

Occt_view& GUI_access::get_view(GUI& gui)
{
  return *gui.m_view;
}

inline void Sketch_access::get_originating_face_snp_pts_3d_(Sketch& sketch, std::vector<gp_Pnt>& out)
{
  sketch.get_originating_face_snp_pts_3d_(out);
}

class View_access
{
 public:
  static void set_view_plane(Occt_view& view, const gp_Pln& pln)
  {
    view.m_curr_view_pln = pln;
  }
};

// Test basic sketch creation and initialization
TEST_F(Sketch_test, CreateSketch)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  EXPECT_EQ(sketch.get_name(), "TestSketch");
  EXPECT_TRUE(sketch.is_visible());
  EXPECT_TRUE(sketch.get_nodes().empty());
  EXPECT_TRUE(planes_equal(sketch.get_plane(), default_plane));
}

// Test node addition and management
TEST_F(Sketch_test, NodeManagement)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  // Add a node
  gp_Pnt2d point(10.0, 10.0);
  size_t   node_idx = sketch.get_nodes().add_new_node(point);

  // Adding the same point should use the existing one;
  gp_Pnt2d              point_copy = point;
  std::optional<size_t> node_idx2  = sketch.get_nodes().try_get_node_idx_snap(point_copy);
  EXPECT_TRUE(node_idx2.has_value());
  EXPECT_EQ(node_idx, *node_idx2);

  EXPECT_EQ(sketch.get_nodes().size(), 1);
  EXPECT_FALSE(sketch.get_nodes().empty());
  EXPECT_TRUE(sketch.get_nodes()[node_idx].IsEqual(point, Precision::Confusion()));
}

// Test edge creation and management
TEST_F(Sketch_test, EdgeManagement)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  // Add two nodes
  gp_Pnt2d point1(0.0, 0.0);
  gp_Pnt2d point2(10.0, 10.0);
  size_t   node1_idx = sketch.get_nodes().add_new_node(point1);
  size_t   node2_idx = sketch.get_nodes().add_new_node(point2);

  // Add an edge between the nodes
  Sketch_access::add_edge_(sketch, point1, point2);

  // Verify edge was created
  EXPECT_FALSE(sketch.get_nodes().empty());
  // Expect 3 nodes because a edge center node is added for snapping purposes.
  EXPECT_EQ(sketch.get_nodes().size(), 3);
}

// Test visibility settings
TEST_F(Sketch_test, VisibilitySettings)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  // Test default visibility
  EXPECT_TRUE(sketch.is_visible());

  // Test changing visibility
  sketch.set_visible(false);
  EXPECT_FALSE(sketch.is_visible());

  // Test face and edge visibility
  sketch.set_show_faces(false);
  sketch.set_show_edges(false);
  // Note: We can't directly test the visual state, but we can verify the settings were applied
}

// Test edge style settings
TEST_F(Sketch_test, EdgeStyleSettings)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  // Test default edge style
  sketch.set_edge_style(Sketch::Edge_style::Full);

  // Add an edge to test style application
  gp_Pnt2d point1(0.0, 0.0);
  gp_Pnt2d point2(10.0, 10.0);
  sketch.get_nodes().add_new_node(point1);
  sketch.get_nodes().add_new_node(point2);
  Sketch_access::add_edge_(sketch, point1, point2);

  // Change edge style
  sketch.set_edge_style(Sketch::Edge_style::Background);
  // Note: We can't directly test the visual style, but we can verify the setting was applied
}

// Test square creation
TEST_F(Sketch_test, CreateSquare)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  // Set mode to square creation
  gui().set_mode(Mode::Sketch_add_square);

  // First point - center of square
  gp_Pnt2d     center(0.0, 0.0);
  ScreenCoords screen_coords(glm::dvec2(center.X(), center.Y()));
  sketch.add_sketch_pt(screen_coords);

  // Second point - center of right edge (defines width and orientation)
  gp_Pnt2d edge_center(10.0, 0.0);
  screen_coords = ScreenCoords(glm::dvec2(edge_center.X(), edge_center.Y()));
  sketch.add_sketch_pt(screen_coords);

  // Finalize the square
  sketch.finalize_elm();

  // Verify that a single face was created
  const auto& faces = Sketch_access::get_faces(sketch);
  EXPECT_EQ(faces.size(), 1) << "Expected exactly one face for the square";

  // Get the face shape
  const auto& face = faces[0];
  EXPECT_FALSE(face.IsNull()) << "Face shape should not be null";

  // Verify the face has the correct number of vertices (5 for a closed square)
  EXPECT_EQ(face->verts_3d.size(), 5) << "Square face should have 5 vertices (4 corners + repeated first vertex)";

  // Convert face to Boost.Geometry polygon and verify its properties
  const TopoDS_Shape& shape = face->Shape();
  EXPECT_EQ(shape.ShapeType(), TopAbs_FACE) << "Shape should be a face";
  boost_geom::polygon_2d boost_poly = to_boost(shape, default_plane);

  // Check that the ring is closed (first and last points should be identical)
  const auto& outer = boost_poly.outer();
  EXPECT_NEAR(outer.front().x(), outer.back().x(), 1e-6) << "Ring should be closed (x)";
  EXPECT_NEAR(outer.front().y(), outer.back().y(), 1e-6) << "Ring should be closed (y)";

  // Check winding order - should be clockwise for Boost.Geometry
  EXPECT_TRUE(is_clockwise(outer)) << "Polygon should be clockwise";

  EXPECT_TRUE(bg::is_valid(boost_poly)) << "Boost polygon should be valid";

  // Verify the area of the square (should be 400 since it's 20x20)
  // Also checks if the polygon is orientated correctly(clockwise)
  double area = bg::area(boost_poly);
  EXPECT_NEAR(area, 400.0, 1e-6) << "Square area should be 400 (20x20)";

  // Verify the number of points in the outer ring (should be 5 for a closed square)
  EXPECT_EQ(boost_poly.outer().size(), 5) << "Boost polygon outer ring should have 5 points (4 corners + repeated first vertex)";

  // Check the polygon to WKT format
  EXPECT_EQ(to_wkt_string(boost_poly), "POLYGON((-10 -10,-10 10,10 10,10 -10,-10 -10))");

  // The nodes should look like:
  // #---@---#
  // |       |
  // @       @
  // |       |
  // #---@---#
  // # normal node
  //
  // @ mid point node

  const Sketch_nodes& nodes = sketch.get_nodes();
  using Node                = Sketch_nodes::Node;
  std::vector<Node> edge_mid_point_nodes;
  std::vector<Node> normal_nodes;
  for (const Node& n : nodes)
    if (!n.deleted)
    {
      if (n.is_midpoint)
        edge_mid_point_nodes.push_back(n);
      else
        normal_nodes.push_back(n);
    }

  EXPECT_EQ(edge_mid_point_nodes.size(), 4);
  EXPECT_EQ(normal_nodes.size(), 4);
  int hi = 0;
}

// Test circle creation
TEST_F(Sketch_test, CreateCircle)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  // Set mode to circle creation
  gui().set_mode(Mode::Sketch_add_circle);

  // First point - center of circle
  gp_Pnt2d     center(0.0, 0.0);
  ScreenCoords screen_coords(glm::dvec2(center.X(), center.Y()));
  sketch.add_sketch_pt(screen_coords);

  // Second point - point on circle (defines radius)
  gp_Pnt2d edge_point(10.0, 0.0);
  screen_coords = ScreenCoords(glm::dvec2(edge_point.X(), edge_point.Y()));
  sketch.add_sketch_pt(screen_coords);

  // Finalize the circle
  sketch.finalize_elm();

  // Verify that a single face was created
  const auto& faces = Sketch_access::get_faces(sketch);
  EXPECT_EQ(faces.size(), 1) << "Expected exactly one face for the circle";

  // Get the face shape
  const auto& face = faces[0];
  EXPECT_FALSE(face.IsNull()) << "Face shape should not be null";

  // Convert face to Boost.Geometry polygon and verify its properties
  const TopoDS_Shape& shape = face->Shape();
  EXPECT_EQ(shape.ShapeType(), TopAbs_FACE) << "Shape should be a face";
  boost_geom::polygon_2d boost_poly = to_boost(shape, default_plane);

  // Check that the ring is closed (first and last points should be identical)
  const auto& outer = boost_poly.outer();
  EXPECT_NEAR(outer.front().x(), outer.back().x(), 1e-6) << "Ring should be closed (x)";
  EXPECT_NEAR(outer.front().y(), outer.back().y(), 1e-6) << "Ring should be closed (y)";

  // Check winding order - should be clockwise for Boost.Geometry
  EXPECT_TRUE(is_clockwise(outer)) << "Polygon should be clockwise";

  EXPECT_TRUE(bg::is_valid(boost_poly)) << "Boost polygon should be valid";

  // Verify the area of the circle (should be pi*r^2 = pi * 10^2 = 100pi)
  double area          = bg::area(boost_poly);
  double expected_area = 100.0 * std::numbers::pi;  // 314.15926535897933
  EXPECT_NEAR(area, expected_area, 1.0) << "Circle area should be 100pi (radius = 10)";

  // Verify the number of points in the outer ring (should be enough points to approximate a circle)
  EXPECT_GT(boost_poly.outer().size(), 20) << "Circle should have enough points to be smooth";

  // Verify the points are roughly equidistant from the center
  double expected_radius = 10.0;
  double tolerance       = 0.1;  // Allow for some approximation error
  for (const auto& point : boost_poly.outer())
  {
    double dx     = point.x() - center.X();
    double dy     = point.y() - center.Y();
    double radius = std::sqrt(dx * dx + dy * dy);
    EXPECT_NEAR(radius, expected_radius, tolerance) << "Point should be on the circle";
  }
}

// Test case 1: Simple rectangle face
TEST_F(Sketch_test, UpdateFaces_SimpleRectangle)
{
  // Define points
  std::vector<gp_Pnt2d> points = {
      { 0,  0}, // 0
      {10,  0}, // 1
      {10, 10}, // 2
      { 0, 10}  // 3
  };

  // Define edges as pairs of indices
  std::vector<std::pair<int, int>> edge_indices = {
      {0, 1},
      {1, 2},
      {2, 3},
      {3, 0}
  };

  auto check_sketch = [&](Sketch& sketch)
  {
    Sketch_access::update_faces_(sketch);
    const auto& faces = Sketch_access::get_faces(sketch);
    EXPECT_EQ(faces.size(), 1);
    // Verify face properties
    const auto& face = faces[0];
    EXPECT_EQ(face->Shape().ShapeType(), TopAbs_FACE);

    boost_geom::polygon_2d boost_poly = to_boost(face->Shape(), sketch.get_plane());
    EXPECT_TRUE(is_clockwise(boost_poly.outer()));

    // Dump the polygon to WKT format
    std::string wkt_str = to_wkt_string(boost_poly);
    EXPECT_EQ(wkt_str, "POLYGON((0 0,0 10,10 10,10 0,0 0))");
  };

  for_all_edge_permutations_(points, edge_indices, gp_Pln(gp::Origin(), gp::DZ()), check_sketch);
}

// Test case 2: Face with hole
TEST_F(Sketch_test, UpdateFaces_FaceWithHole)
{
  Sketch sketch("test_sketch", view(), gp_Pln(gp::Origin(), gp::DZ()));

  // Define points
  std::vector<gp_Pnt2d> points = {
      { 0,  0}, // 0
      {20,  0}, // 1
      {20, 20}, // 2
      { 0, 20}, // 3
      { 5,  5}, // 4
      {15,  5}, // 5
      {15, 15}, // 6
      { 5, 15}  // 7
  };

  // Define edges as pairs of indices
  std::vector<std::pair<int, int>> edge_indices = {
      {0, 1},
      {1, 2},
      {2, 3},
      {3, 0}, // Outer rectangle
      {4, 5},
      {5, 6},
      {6, 7},
      {7, 4}  // Inner rectangle (hole)
  };

  auto check_sketch = [&](Sketch& sketch)
  {
    Sketch_access::update_faces_(sketch);
    const auto& faces = Sketch_access::get_faces(sketch);
    EXPECT_EQ(faces.size(), 2);
    boost_geom::polygon_2d face_with_hole;
    boost_geom::polygon_2d hole_face;
    for (auto& face : faces)
    {
      // Verify face properties
      EXPECT_EQ(face->Shape().ShapeType(), TopAbs_FACE);
      boost_geom::polygon_2d boost_poly = to_boost(face->Shape(), sketch.get_plane());
      EXPECT_TRUE(is_clockwise(boost_poly.outer()));
      EXPECT_FALSE(boost_poly.outer().empty());
      if (boost_poly.inners().size())
      {
        EXPECT_TRUE(face_with_hole.outer().empty());
        std::swap(face_with_hole, boost_poly);
      }
      else
      {
        EXPECT_TRUE(hole_face.outer().empty());
        std::swap(hole_face, boost_poly);
      }
    }

    EXPECT_EQ(to_wkt_string(hole_face), "POLYGON((5 5,5 15,15 15,15 5,5 5))");
    EXPECT_EQ(to_wkt_string(face_with_hole), "POLYGON((0 0,0 20,20 20,20 0,0 0),(5 5,15 5,15 15,5 15,5 5))");
  };

  add_edges_from_indices_(points, edge_indices, gp_Pln(gp::Origin(), gp::DZ()), check_sketch);
}

// Test case 3: Multiple faces
TEST_F(Sketch_test, UpdateFaces_MultipleFaces)
{
  Sketch sketch("test_sketch", view(), gp_Pln(gp::Origin(), gp::DZ()));

  // Create two separate rectangles
  // Rectangle 1
  gp_Pnt2d p1(0, 0);
  gp_Pnt2d p2(10, 0);
  gp_Pnt2d p3(10, 10);
  gp_Pnt2d p4(0, 10);

  // Rectangle 2
  gp_Pnt2d p5(20, 0);
  gp_Pnt2d p6(30, 0);
  gp_Pnt2d p7(30, 10);
  gp_Pnt2d p8(20, 10);

  // Add edges for first rectangle
  Sketch_access::add_edge_(sketch, p1, p2);
  Sketch_access::add_edge_(sketch, p2, p3);
  Sketch_access::add_edge_(sketch, p3, p4);
  Sketch_access::add_edge_(sketch, p4, p1);

  // Add edges for second rectangle
  Sketch_access::add_edge_(sketch, p5, p6);
  Sketch_access::add_edge_(sketch, p6, p7);
  Sketch_access::add_edge_(sketch, p7, p8);
  Sketch_access::add_edge_(sketch, p8, p5);

  Sketch_access::update_faces_(sketch);

  // Verify two faces were created
  const auto& faces = Sketch_access::get_faces(sketch);
  EXPECT_EQ(faces.size(), 2);

  // Verify each face's properties
  for (const auto& face : faces)
  {
    EXPECT_EQ(face->Shape().ShapeType(), TopAbs_FACE);
    EXPECT_EQ(face->verts_3d.size(), 5);
  }
}

// Test case 4: Invalid face (non-closed shape)
TEST_F(Sketch_test, UpdateFaces_InvalidFace)
{
  Sketch sketch("test_sketch", view(), gp_Pln(gp::Origin(), gp::DZ()));

  // Add three edges that don't form a closed shape
  gp_Pnt2d p1(0, 0);
  gp_Pnt2d p2(10, 0);
  gp_Pnt2d p3(10, 10);

  Sketch_access::add_edge_(sketch, p1, p2);
  Sketch_access::add_edge_(sketch, p2, p3);

  Sketch_access::update_faces_(sketch);

  // Verify no faces were created
  const auto& faces = Sketch_access::get_faces(sketch);
  EXPECT_EQ(faces.size(), 0);
}

// Test case 5: Face with arc edges
TEST_F(Sketch_test, UpdateFaces_FaceWithArcs)
{
  Sketch sketch("test_sketch", view(), gp_Pln(gp::Origin(), gp::DZ()));

  // Create a face with straight edges and arc edges
  gp_Pnt2d p1(0, 0);
  gp_Pnt2d p2(10, 0);
  gp_Pnt2d p3(10, 10);
  gp_Pnt2d p4(0, 10);
  gp_Pnt2d p5(5, 5);  // Center point for arc

  // Add straight edges
  Sketch_access::add_edge_(sketch, p1, p2);
  Sketch_access::add_edge_(sketch, p2, p3);
  Sketch_access::add_edge_(sketch, p3, p4);

  // Add arc edge using Sketch_access
  Sketch_access::add_arc_circle_(sketch, p4, p5, p1);

  Sketch_access::update_faces_(sketch);

  // Verify one face was created
  const auto& faces = Sketch_access::get_faces(sketch);
  EXPECT_EQ(faces.size(), 1);

  // Verify face properties
  ASSERT_FALSE(faces.empty());
  const auto& face = faces[0];
  EXPECT_EQ(face->Shape().ShapeType(), TopAbs_FACE);

  /*
  // Verify face has both straight and curved edges
  TopExp_Explorer edge_explorer(face->Shape(), TopAbs_EDGE);
  int edge_count = 0;
  int curved_edge_count = 0;
  for (; edge_explorer.More(); edge_explorer.Next()) {
      edge_count++;
      TopoDS_Edge edge = TopoDS::Edge(edge_explorer.Current());
      if (!BRep_Tool::IsGeometric(edge)) {
          curved_edge_count++;
      }
  }
  EXPECT_EQ(edge_count, 4); // Total edges
  EXPECT_GT(curved_edge_count, 0); // At least one curved edge
  */
}

TEST_F(Sketch_test, OriginatingFaceSnapPointsSquare)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());

  // Create a wire box centered at the origin, 20x10 in size
  gp_Pnt      center(0, 0, 0);
  double      width      = 20.0;
  double      height     = 10.0;
  TopoDS_Wire outer_wire = create_wire_box(default_plane, center, width, height);

  // Create the Sketch using the wire as the originating face
  Sketch sketch("OriginatingFace", view(), default_plane, outer_wire);

  // Check that the sketch is visible
  EXPECT_TRUE(sketch.is_visible());

  // Optionally, check that the snap points include the wire's vertices
  std::vector<gp_Pnt> snap_pts;
  Sketch_access::get_originating_face_snp_pts_3d_(sketch, snap_pts);
  sort_pnts(snap_pts);

  std::vector<gp_Pnt> expected = {
      gp_Pnt(-10, -5, 0),
      gp_Pnt(-10, 0, 0),
      gp_Pnt(-10, 5, 0),
      gp_Pnt(0, -5, 0),
      gp_Pnt(0, 5, 0),
      gp_Pnt(10, -5, 0),
      gp_Pnt(10, 0, 0),
      gp_Pnt(10, 5, 0),
  };

  ASSERT_EQ(snap_pts.size(), expected.size());
  for (size_t i = 0; i < expected.size(); ++i)
    EXPECT_TRUE(snap_pts[i].IsEqual(expected[i], Precision::Confusion()))
        << "Mismatch at index " << i << ": got " << snap_pts[i].X() << "," << snap_pts[i].Y();
}

TEST_F(Sketch_test, OriginatingFaceSnapPointsCircle)
{
  gp_Pln   default_plane(gp::Origin(), gp::DZ());
  gp_Pnt2d center(0.0, 0.0);
  gp_Pnt2d edge_point(10.0, 0.0);  // Radius = 10

  // Create a circular wire as the originating face
  TopoDS_Wire circle_wire = make_circle_wire(default_plane, center, edge_point);

  // Create the Sketch using the wire as the originating face
  Sketch sketch("OriginatingFaceCircle", view(), default_plane, circle_wire);

  // Check that the sketch is visible
  EXPECT_TRUE(sketch.is_visible());

  // Get the snap points from the originating face
  std::vector<gp_Pnt> snap_pts;
  Sketch_access::get_originating_face_snp_pts_3d_(sketch, snap_pts);
  sort_pnts(snap_pts);

  // For a circle, we expect snap points at the cardinal directions (0, 90, 180, 270 degrees)
  std::vector<gp_Pnt> expected = {
      gp_Pnt(10, 0, 0),   // 0 degrees (right)
      gp_Pnt(0, 10, 0),   // 90 degrees (top)
      gp_Pnt(-10, 0, 0),  // 180 degrees (left)
      gp_Pnt(0, -10, 0),  // 270 degrees (bottom)
  };

  // The actual implementation may provide more points (e.g., more subdivisions).
  // We'll check that all expected points are present in the snap_pts.
  for (const auto& exp : expected)
  {
    bool found = false;
    for (const auto& pt : snap_pts)
    {
      if (pt.IsEqual(exp, Precision::Confusion()))
      {
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found) << "Expected snap point at (" << exp.X() << ", " << exp.Y() << ")";
  }
}

TEST_F(Sketch_test, ExtrudeSketchFace_EzyCad)
{
#if 0  // Not working
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  // Create a rectangle face in the sketch
  gp_Pnt2d p1(0, 0), p2(10, 0), p3(10, 5), p4(0, 5);
  Sketch_access::add_edge_(sketch, p1, p2);
  Sketch_access::add_edge_(sketch, p2, p3);
  Sketch_access::add_edge_(sketch, p3, p4);
  Sketch_access::add_edge_(sketch, p4, p1);

  Sketch_access::update_faces_(sketch);
  const auto& faces = Sketch_access::get_faces(sketch);
  ASSERT_EQ(faces.size(), 1);

  // Tilt the view plane a little
  gp_Vec tilted_vec(0, 0.1, 1);      // Slightly off Z
  gp_Dir tilted_normal(tilted_vec);  // gp_Dir is always normalized
  gp_Pln tilted_plane(gp::Origin(), tilted_normal);
  View_access::set_view_plane(view(), tilted_plane);

  // Convert a 3D point on the face to screen coordinates
  gp_Pnt       face_center(5, 2.5, 0);
  ScreenCoords screen_coords = view().get_screen_coords(face_center);

  // Set the mode to extrusion (if required)
  gui().set_mode(Mode::Sketch_face_extrude);

  // Call the extrusion function
  view().sketch_face_extrude(screen_coords);

  // Now, check that a new shape was created and is a solid
  auto& shapes = view().get_shapes();
  ASSERT_FALSE(shapes.empty());

  const auto&         extruded_shape = shapes.back();
  const TopoDS_Shape& topo_shape     = extruded_shape->Shape();

  EXPECT_EQ(topo_shape.ShapeType(), TopAbs_SOLID);

  // Optionally, check the bounding box or volume as before
#endif
}

TEST_F(Sketch_test, SquareTwoArcs)
{
#if 0  // TODO currently failing
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  // Define square corners
  gp_Pnt2d p1(-10, -10);
  gp_Pnt2d p2(10, -10);
  gp_Pnt2d p3(10, 10);
  gp_Pnt2d p4(-10, 10);

  // Add square edges
  Sketch_access::add_edge_(sketch, p1, p2);
  Sketch_access::add_edge_(sketch, p2, p3);
  Sketch_access::add_edge_(sketch, p3, p4);
  Sketch_access::add_edge_(sketch, p4, p1);

  // Compute the center of the square
  gp_Pnt2d center(0, 0);

  // Add first arc: from p1 to p2, arc through center
  //Sketch_access::add_arc_circle_(sketch, p1, p2, center);

  // Add second arc: from p1 to p2, arc through center (again)
  Sketch_access::add_arc_circle_(sketch, p1, p2, center);

  // Update faces
  Sketch_access::update_faces_(sketch);

  // There should be one face (the square with two arcs replacing one edge)
  const auto& faces = Sketch_access::get_faces(sketch);
  EXPECT_EQ(faces.size(), 1);

  // Check the face is a valid TopoDS_Face
  const auto& face = faces[0];
  EXPECT_EQ(face->Shape().ShapeType(), TopAbs_FACE);

  // Convert to Boost.Geometry polygon for further checks
  boost_geom::polygon_2d boost_poly = to_boost(face->Shape(), default_plane);

  // Check that the ring is closed
  const auto& outer = boost_poly.outer();
  EXPECT_NEAR(outer.front().x(), outer.back().x(), 1e-6);
  EXPECT_NEAR(outer.front().y(), outer.back().y(), 1e-6);

  // Check the polygon is valid
  EXPECT_TRUE(bg::is_valid(boost_poly));

#endif
}

// Helper to add all permutations of edges (with both orientations) to a sketch and call a lambda
template <typename Callback>
void Sketch_test::for_all_edge_permutations_(
    const std::vector<gp_Pnt2d>&            points,
    const std::vector<std::pair<int, int>>& edge_indices,
    const gp_Pln&                           plane,
    Callback&&                              callback)
{
  std::vector<std::pair<int, int>> edges = edge_indices;
  std::vector<bool>                orientations(edges.size(), false);  // false: as-is, true: reversed

  // Generate all permutations of edge order
  do
  {
    // For each permutation, generate all combinations of orientations
    size_t n = edges.size();
    for (size_t mask = 0; mask < (1ull << n); ++mask)
    {
      // Prepare oriented edges for this combination
      std::vector<std::pair<int, int>> oriented_edges = edges;
      for (size_t i = 0; i < n; ++i)
        if (mask & (1ull << i))
          std::swap(oriented_edges[i].first, oriented_edges[i].second);

      // Create a new sketch and add edges in this order/orientation
      Sketch sketch("test_sketch", view(), plane);
      for (const auto& [a, b] : oriented_edges)
        Sketch_access::add_edge_(sketch, points[a], points[b]);

      callback(sketch);
    }
  } while (std::next_permutation(edges.begin(), edges.end()));
}

// Helper to add edges to a sketch from a vector of points and edge index pairs
template <typename Callback>
void Sketch_test::add_edges_from_indices_(
    const std::vector<gp_Pnt2d>&            points,
    const std::vector<std::pair<int, int>>& edge_indices,
    const gp_Pln&                           plane,
    Callback&&                              callback)
{
  Sketch sketch("test_sketch", view(), plane);
  for (const auto& [i, j] : edge_indices)
    Sketch_access::add_edge_(sketch, points[i], points[j]);

  callback(sketch);
}
