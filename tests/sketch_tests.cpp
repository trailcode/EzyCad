#include "sketch_test_fixture.h"

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <algorithm>
#include <numbers>
#include <vector>

#include "sketch_edge.h"
#include "sketch_json.h"
#include "sketch_nodes.h"
#include "utl_geom.h"

using namespace glm;

// Test basic sketch creation and initialization
TEST_F(Sketch_test, CreateSketch)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  EXPECT_EQ(sketch.get_name(), "TestSketch");
  EXPECT_TRUE(sketch.is_visible());
  EXPECT_EQ(sketch.get_nodes().size(), 1u);
  EXPECT_TRUE(sketch.get_nodes()[0].origin);
  EXPECT_TRUE(sketch.get_nodes()[0].permanent);
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

  EXPECT_EQ(sketch.get_nodes().size(), 2u);
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
  // Origin node + 2 endpoints + 1 midpoint (midpoints enabled in fixture).
  EXPECT_EQ(sketch.get_nodes().size(), 4u);
}

// Test that adding a single edge via add_edge_ creates exactly three nodes:
// the two endpoints (user nodes) + one automatically created midpoint node for snapping.
// Verifies positions and the midpoint flag.
TEST_F(Sketch_test, AddSingleEdge_CreatesThreeCorrectNodes)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  // Enable midpoint creation for this test (tests the "add midpoints" behavior)
  sketch.set_add_mid_pt_edges(true);

  gp_Pnt2d p1(0.0, 0.0);
  gp_Pnt2d p2(10.0, 5.0);

  // Add a single edge directly (this is the low-level path used by many creation tools)
  Sketch_access::add_edge_(sketch, p1, p2);

  const Sketch_nodes& nodes = sketch.get_nodes();
  EXPECT_EQ(nodes.size(), 3) << "Adding one edge should create two endpoints + one midpoint node";

  // Node order from add_edge_raw_:
  // 0: get_node_exact(p1) -> first endpoint
  // 1: get_node_exact(p2) -> second endpoint
  // 2: add_new_node(midpoint) inside update_edge_end_pt_
  EXPECT_TRUE(nodes[0].IsEqual(p1, Precision::Confusion())) << "Node 0 should be p1";
  EXPECT_FALSE(nodes[0].midpoint) << "Endpoint nodes should not be marked as midpoint";

  EXPECT_TRUE(nodes[1].IsEqual(p2, Precision::Confusion())) << "Node 1 should be p2";
  EXPECT_FALSE(nodes[1].midpoint);

  gp_Pnt2d expected_mid((p1.X() + p2.X()) * 0.5, (p1.Y() + p2.Y()) * 0.5);
  EXPECT_TRUE(nodes[2].IsEqual(expected_mid, Precision::Confusion())) << "Node 2 should be the exact midpoint";
  EXPECT_TRUE(nodes[2].midpoint) << "The auto-created center node must be marked as midpoint for snapping";
}

// Test the new "add midpoints to linear edges" option (default off).
// Verifies that linear edges do not get auto midpoints by default,
// and that the static setter controls it for add_edge_ (used by line/multi-line tools).
TEST_F(Sketch_test, AddLinearEdge_MidpointOption)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  gp_Pnt2d p1(0.0, 0.0);
  gp_Pnt2d p2(10.0, 0.0);

  // Force off to test no-mid default behavior (fixture forces on for other tests)
  Sketch::set_add_mid_pt_edges(false);
  EXPECT_FALSE(Sketch::get_add_mid_pt_edges()) << "Should be false right before add";

  Sketch_access::add_edge_(sketch, p1, p2);

  size_t nodes_after_first = sketch.get_nodes().size();
  EXPECT_EQ(nodes_after_first, 2) << "Default (off): only endpoints, no auto midpoint";
  EXPECT_EQ(Sketch_access::get_linear_edge_count(sketch), 1);

  // Check the (only) edge has no mid
  bool has_mid = false;
  for (const auto& e : Sketch_access::get_edges(sketch))
    if (e.node_idx_mid.has_value())
      has_mid = true;
  
  EXPECT_FALSE(has_mid);

  // Turn the option on
  Sketch::set_add_mid_pt_edges(true);
  EXPECT_TRUE(Sketch::get_add_mid_pt_edges());

  gp_Pnt2d p3(0.0, 10.0);
  gp_Pnt2d p4(10.0, 10.0);
  Sketch_access::add_edge_(sketch, p3, p4);

  // Should add 2 endpoints + 1 mid = +3
  EXPECT_EQ(sketch.get_nodes().size(), nodes_after_first + 3);

  // Find the second edge and verify it has a mid
  auto it = Sketch_access::get_edges(sketch).begin();
  std::advance(it, 1); // second edge
  EXPECT_TRUE(it->node_idx_mid.has_value());
  const gp_Pnt2d& mid = sketch.get_nodes()[*it->node_idx_mid];
  gp_Pnt2d        expected_mid(5.0, 10.0);
  EXPECT_TRUE(mid.IsEqual(expected_mid, Precision::Confusion()));
  EXPECT_TRUE(sketch.get_nodes()[*it->node_idx_mid].midpoint);

  // Verify JSON shape: no mid => 2-element edge array; with mid => 3 elements
  nlohmann::json j_no_mid = Sketch_json::to_json(sketch, view().asset_store()); // current state has one with mid
  // To test no-mid JSON, recreate a sketch with flag off
  Sketch sketch2("TestSketch2", view(), default_plane);
  Sketch::set_add_mid_pt_edges(false);
  Sketch_access::add_edge_(sketch2, p1, p2);
  nlohmann::json j2 = Sketch_json::to_json(sketch2, view().asset_store());
  EXPECT_EQ(j2["edges"][0].size(), 2u) << "Edge without mid should serialize as 2-element array [a,b]";

  Sketch::set_add_mid_pt_edges(true);
  Sketch_access::add_edge_(sketch2, p3, p4);
  nlohmann::json j3 = Sketch_json::to_json(sketch2, view().asset_store());
  EXPECT_EQ(j3["edges"][1].size(), 3u) << "Edge with mid should serialize as 3-element array [a,b,mid]";

  // Turn off again for subsequent tests (though fixture resets)
  Sketch::set_add_mid_pt_edges(false);
}

TEST_F(Sketch_test, AddArc_MidpointOption)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  const gp_Pnt2d start(0.0, 0.0);
  const gp_Pnt2d pick(5.0, 5.0);
  const gp_Pnt2d end(10.0, 0.0);

  Sketch::set_add_mid_pt_edges(false);
  Sketch_access::add_arc_circle_(sketch, start, end, pick);

  EXPECT_EQ(sketch.get_nodes().size(), 3u) << "Off: start, end, and bulge pick only";
  bool has_arc_mid = false;
  for (const auto& e : Sketch_access::get_edges(sketch))
    if (e.node_idx_arc_pt.has_value())
      has_arc_mid = true;

  EXPECT_FALSE(has_arc_mid);

  Sketch::set_add_mid_pt_edges(true);
  const gp_Pnt2d start2(0.0, 10.0);
  const gp_Pnt2d pick2(5.0, 15.0);
  const gp_Pnt2d end2(10.0, 10.0);
  Sketch_access::add_arc_circle_(sketch, start2, end2, pick2);

  const Sketch_edge* arc_with_mid = nullptr;
  for (const auto& e : Sketch_access::get_edges(sketch))
    if (sketch_edge_is_arc(e) && e.node_idx_arc_pt.has_value())
      arc_with_mid = &e;

  ASSERT_NE(arc_with_mid, nullptr);
  const Sketch_nodes::Node& mid_node = sketch.get_nodes()[*arc_with_mid->node_idx_arc_pt];
  EXPECT_TRUE(mid_node.midpoint);
  const gp_Pnt2d expected_mid = arc_curve_midpoint_2d(TopoDS::Edge(arc_with_mid->shp->Shape()), default_plane);
  EXPECT_TRUE(mid_node.IsEqual(expected_mid, Precision::Confusion()))
      << "Arc midpoint snap node should lie on the curve at parametric half";

  Sketch::set_add_mid_pt_edges(false);
}

// Test add line edge with "place from center": Tab length is the full edge span.
TEST_F(Sketch_test, AddEdgeFromCenter_SecondClickFullSpan)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  Sketch::set_add_mid_pt_edges(false);
  Sketch::set_edge_from_center(true);
  gui().set_mode(Mode::Sketch_add_edge);

  sketch.add_sketch_pt(ScreenCoords(dvec2(0.0, 0.0)));
  sketch.add_sketch_pt(ScreenCoords(dvec2(5.0, 0.0)));

  EXPECT_EQ(Sketch_access::get_linear_edge_count(sketch), 1);

  bool found = false;
  for (const auto& e : Sketch_access::get_edges(sketch))
  {
    if (!sketch_edge_is_linear(e))
      continue;

    const gp_Pnt2d& a    = sketch.get_nodes()[e.node_idx_a];
    const gp_Pnt2d& b    = sketch.get_nodes()[*e.node_idx_b];
    const double    xmin = std::min(a.X(), b.X());
    const double    xmax = std::max(a.X(), b.X());
    if (std::abs(a.Y()) < Precision::Confusion() && std::abs(b.Y()) < Precision::Confusion() &&
        std::abs(xmin + 5.0) < Precision::Confusion() && std::abs(xmax - 5.0) < Precision::Confusion())
    {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found) << "Center (0,0) + direction click (5,0) should create edge (-5,0) to (5,0)";

  Sketch::set_edge_from_center(false);
}

TEST_F(Sketch_test, AddEdgeFromCenter_DimensionUsesFullLength)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  Sketch::set_add_mid_pt_edges(false);
  Sketch::set_edge_from_center(true);
  gui().set_mode(Mode::Sketch_add_edge);

  sketch.add_sketch_pt(ScreenCoords(dvec2(0.0, 0.0)));
  sketch.sketch_pt_move(ScreenCoords(dvec2(1.0, 0.0)));

  Sketch_access::set_entered_edge_len(sketch, gp_Dir2d(1.0, 0.0), 10.0);
  sketch.on_enter();

  EXPECT_EQ(Sketch_access::get_linear_edge_count(sketch), 1);

  bool found = false;
  for (const auto& e : Sketch_access::get_edges(sketch))
  {
    if (!sketch_edge_is_linear(e))
      continue;

    const gp_Pnt2d& a    = sketch.get_nodes()[e.node_idx_a];
    const gp_Pnt2d& b    = sketch.get_nodes()[*e.node_idx_b];
    const double    xmin = std::min(a.X(), b.X());
    const double    xmax = std::max(a.X(), b.X());
    if (std::abs(a.Y()) < Precision::Confusion() && std::abs(b.Y()) < Precision::Confusion() &&
        std::abs(xmin + 5.0) < Precision::Confusion() && std::abs(xmax - 5.0) < Precision::Confusion())
    {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found) << "Full length 10 from center (0,0) along +X should create edge (-5,0) to (5,0)";

  Sketch::set_edge_from_center(false);
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
  ScreenCoords screen_coords(dvec2(center.X(), center.Y()));
  sketch.add_sketch_pt(screen_coords);

  // Second point - center of right edge (defines width and orientation)
  gp_Pnt2d edge_center(10.0, 0.0);
  screen_coords = ScreenCoords(dvec2(edge_center.X(), edge_center.Y()));
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

  // Convert to our polygon type (pure C++ re-implementation of the previous
  // (pure C++ polygon conversion) and verify its properties.
  const TopoDS_Shape& shape = face->Shape();
  EXPECT_EQ(shape.ShapeType(), TopAbs_FACE) << "Shape should be a face";
  ezy_geom::polygon_2d poly = to_boost(shape, default_plane);

  // Check that the ring is closed (first and last points should be identical)
  const auto& outer = poly.outer();
  EXPECT_NEAR(outer.front().x(), outer.back().x(), 1e-6) << "Ring should be closed (x)";
  EXPECT_NEAR(outer.front().y(), outer.back().y(), 1e-6) << "Ring should be closed (y)";

  // Check winding order - should be clockwise for outer rings
  EXPECT_TRUE(is_clockwise(outer)) << "Polygon should be clockwise";

  EXPECT_TRUE(ezy_geom::is_valid(poly)) << "Polygon should be valid";

  // Verify the area of the square (should be 400 since it's 20x20)
  // Also checks if the polygon is orientated correctly (clockwise)
  double area = ezy_geom::area(poly);
  EXPECT_NEAR(area, 400.0, 1e-6) << "Square area should be 400 (20x20)";

  // Verify the number of points in the outer ring (should be 5 for a closed square)
  EXPECT_EQ(poly.outer().size(), 5) << "Polygon outer ring should have 5 points (4 corners + repeated first vertex)";

  // Check the polygon to WKT format
  EXPECT_EQ(to_wkt_string(poly), "POLYGON((-10 -10,-10 10,10 10,10 -10,-10 -10))");

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
      if (n.midpoint)
        edge_mid_point_nodes.push_back(n);
      else
        normal_nodes.push_back(n);
    }

  EXPECT_EQ(edge_mid_point_nodes.size(), 4u);
  EXPECT_EQ(normal_nodes.size(), 5u); // 4 corners + sketch origin at center
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
  ScreenCoords screen_coords(dvec2(center.X(), center.Y()));
  sketch.add_sketch_pt(screen_coords);

  // Second point - point on circle (defines radius)
  gp_Pnt2d edge_point(10.0, 0.0);
  screen_coords = ScreenCoords(dvec2(edge_point.X(), edge_point.Y()));
  sketch.add_sketch_pt(screen_coords);

  // Finalize the circle
  sketch.finalize_elm();

  // Verify that a single face was created
  const auto& faces = Sketch_access::get_faces(sketch);
  EXPECT_EQ(faces.size(), 1) << "Expected exactly one face for the circle";

  // Get the face shape
  const auto& face = faces[0];
  EXPECT_FALSE(face.IsNull()) << "Face shape should not be null";

  const TopoDS_Shape& shape = face->Shape();
  EXPECT_EQ(shape.ShapeType(), TopAbs_FACE) << "Shape should be a face";

  GProp_GProps props;
  BRepGProp::SurfaceProperties(shape, props);
  const double expected_area = 100.0 * std::numbers::pi;
  EXPECT_NEAR(props.Mass(), expected_area, 1.0) << "Circle area should be 100pi (radius = 10)";
}

// Snap points from a square originating face
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
      gp_Pnt(-10, -5, 0), gp_Pnt(-10, 0, 0), gp_Pnt(-10, 5, 0), gp_Pnt(0, -5, 0),
      gp_Pnt(0, 5, 0),    gp_Pnt(10, -5, 0), gp_Pnt(10, 0, 0),  gp_Pnt(10, 5, 0),
  };

  ASSERT_EQ(snap_pts.size(), expected.size());
  for (size_t i = 0; i < expected.size(); ++i)
    EXPECT_TRUE(snap_pts[i].IsEqual(expected[i], Precision::Confusion()))
        << "Mismatch at index " << i << ": got " << snap_pts[i].X() << "," << snap_pts[i].Y();
}

TEST_F(Sketch_test, SketchOriginNodePlane)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("PlaneOrigin", view(), default_plane);

  std::optional<size_t> origin_idx;
  for (size_t i = 0; i < sketch.get_nodes().size(); ++i)
  {
    const Sketch_nodes::Node& n = sketch.get_nodes()[i];
    if (!n.deleted && n.origin)
    {
      origin_idx = i;
      break;
    }
  }

  ASSERT_TRUE(origin_idx.has_value());
  const Sketch_nodes::Node& origin = sketch.get_nodes()[*origin_idx];
  EXPECT_TRUE(origin.permanent);
  EXPECT_NEAR(origin.X(), 0.0, Precision::Confusion());
  EXPECT_NEAR(origin.Y(), 0.0, Precision::Confusion());
  EXPECT_EQ(origin.name, "Origin");
}

TEST_F(Sketch_test, SketchOriginNodeFromFaceBBoxCenter)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());

  gp_Pnt      center(5.0, 3.0, 0.0);
  TopoDS_Wire outer_wire = create_wire_box(default_plane, center, 20.0, 10.0);
  Sketch      sketch("FaceOrigin", view(), default_plane, outer_wire);

  std::optional<size_t> origin_idx;
  for (size_t i = 0; i < sketch.get_nodes().size(); ++i)
  {
    const Sketch_nodes::Node& n = sketch.get_nodes()[i];
    if (!n.deleted && n.origin)
    {
      origin_idx = i;
      break;
    }
  }

  ASSERT_TRUE(origin_idx.has_value());
  const Sketch_nodes::Node& origin = sketch.get_nodes()[*origin_idx];
  EXPECT_TRUE(origin.permanent);
  EXPECT_NEAR(origin.X(), 5.0, Precision::Confusion());
  EXPECT_NEAR(origin.Y(), 3.0, Precision::Confusion());
}

TEST_F(Sketch_test, SketchOriginNodeSurvivesOnMode)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());

  gp_Pnt      center(5.0, 3.0, 0.0);
  TopoDS_Wire outer_wire = create_wire_box(default_plane, center, 20.0, 10.0);
  Sketch      sketch("FaceOrigin", view(), default_plane, outer_wire);

  // Same path as create_sketch_from_planar_face_ -> set_mode(Sketch_inspection_mode).
  sketch.on_mode();

  std::optional<size_t> origin_idx;
  for (size_t i = 0; i < sketch.get_nodes().size(); ++i)
  {
    const Sketch_nodes::Node& n = sketch.get_nodes()[i];
    if (!n.deleted && n.origin)
    {
      origin_idx = i;
      break;
    }
  }

  ASSERT_TRUE(origin_idx.has_value());
  EXPECT_NEAR(sketch.get_nodes()[*origin_idx].X(), 5.0, Precision::Confusion());
  EXPECT_NEAR(sketch.get_nodes()[*origin_idx].Y(), 3.0, Precision::Confusion());
}

TEST_F(Sketch_test, SetOriginPt)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("OriginEdit", view(), default_plane);

  sketch.set_origin_pt(gp_Pnt2d(12.0, -3.5));
  EXPECT_NEAR(sketch.origin_pt().X(), 12.0, Precision::Confusion());
  EXPECT_NEAR(sketch.origin_pt().Y(), -3.5, Precision::Confusion());

  sketch.reset_origin_pt();
  EXPECT_NEAR(sketch.origin_pt().X(), 0.0, Precision::Confusion());
  EXPECT_NEAR(sketch.origin_pt().Y(), 0.0, Precision::Confusion());
}

TEST_F(Sketch_test, OriginNodeNotDeletable)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("OriginNoDelete", view(), default_plane);

  std::optional<size_t> origin_idx;
  for (size_t i = 0; i < sketch.get_nodes().size(); ++i)
    if (!sketch.get_nodes()[i].deleted && sketch.get_nodes()[i].origin)
    {
      origin_idx = i;
      break;
    }

  ASSERT_TRUE(origin_idx.has_value());
  Sketch_access::remove_permanent_node_mark_(sketch, *origin_idx);
  EXPECT_FALSE(sketch.get_nodes()[*origin_idx].deleted);
}

TEST_F(Sketch_test, HiddenOriginExcludedFromSnap)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("HiddenOriginSnap", view(), default_plane);

  std::vector<gp_Pnt> pts;
  sketch.get_nodes().get_snap_pts_3d(pts);
  ASSERT_EQ(pts.size(), 1u);

  sketch.set_show_origin_marker(false);
  EXPECT_FALSE(sketch.get_nodes().origin_snap_enabled());

  pts.clear();
  sketch.get_nodes().get_snap_pts_3d(pts);
  EXPECT_TRUE(pts.empty());

  gp_Pnt2d              near_origin(0.01, 0.01);
  std::optional<size_t> snap_idx = sketch.get_nodes().try_get_node_idx_snap(near_origin);
  EXPECT_FALSE(snap_idx.has_value());
}

TEST_F(Sketch_test, OriginatingFaceSnapPointsCircle)
{
  gp_Pln   default_plane(gp::Origin(), gp::DZ());
  gp_Pnt2d center(0.0, 0.0);
  gp_Pnt2d edge_point(10.0, 0.0); // Radius = 10

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

  // For a circle, the implementation extracts:
  // - Start vertex (at angle 0, where the circle starts)
  // - Midpoint (at angle π, 180 degrees)
  // - End vertex (same as start for closed circle, so not added again)
  // So we expect exactly 2 points: start vertex and midpoint
  std::vector<gp_Pnt> expected = {
      gp_Pnt(-10, 0, 0), // Midpoint at 180 degrees
      gp_Pnt(10, 0, 0),  // Start vertex at 0 degrees (edge_point)
  };

  ASSERT_EQ(snap_pts.size(), expected.size());
  for (size_t i = 0; i < expected.size(); ++i)
    EXPECT_TRUE(snap_pts[i].IsEqual(expected[i], Precision::Confusion()))
        << "Mismatch at index " << i << ": got (" << snap_pts[i].X() << ", " << snap_pts[i].Y() << ", " << snap_pts[i].Z()
        << "), expected (" << expected[i].X() << ", " << expected[i].Y() << ", " << expected[i].Z() << ")";
}

TEST_F(Sketch_test, ExtrudeSketchFace_EzyCad)
{
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

  gui().set_mode(Mode::Sketch_face_extrude);

  // Headless/offscreen viewers cannot AIS-pick faces via MoveTo; inject the face directly.
  Shp_extrude_access::begin_face_extrude(view().shp_extrude(), faces[0], 5.0);
  ASSERT_TRUE(view().shp_extrude().has_active_extrusion());

  view().shp_extrude().finalize();

  auto& shapes = view().get_shapes();
  ASSERT_FALSE(shapes.empty());
  EXPECT_EQ(shapes.back()->Shape().ShapeType(), TopAbs_SOLID);
}

TEST_F(Sketch_test, AddNode_splits_linear_edge_interior)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  Sketch_access::add_edge_(sketch, gp_Pnt2d(0.0, 0.0), gp_Pnt2d(20.0, 0.0));
  Sketch_access::update_faces_(sketch);

  ASSERT_EQ(Sketch_access::get_edge_count(sketch), 1u);

  gui().set_mode(Mode::Sketch_add_node);
  // Snap to an existing endpoint first (rubber band), then place the new node on the edge interior.
  ScreenCoords anchor(dvec2(0.0, 0.0));
  sketch.add_sketch_pt(anchor);
  ScreenCoords interior(dvec2(7.0, 0.0));
  sketch.add_sketch_pt(interior);

  EXPECT_EQ(Sketch_access::get_edge_count(sketch), 2u) << "Add node on edge interior should replace one edge with two";

  bool found_0_7  = false;
  bool found_7_20 = false;
  for (const auto& e : Sketch_access::get_edges(sketch))
  {
    if (!sketch_edge_is_linear(e))
      continue;

    const gp_Pnt2d& pa = sketch.get_nodes()[e.node_idx_a];
    const gp_Pnt2d& pb = sketch.get_nodes()[*e.node_idx_b];
    if (std::abs(pa.Y()) < 1e-6 && std::abs(pb.Y()) < 1e-6)
    {
      double x0 = std::min(pa.X(), pb.X());
      double x1 = std::max(pa.X(), pb.X());
      if (std::abs(x0 - 0.0) < 1e-6 && std::abs(x1 - 7.0) < 1e-6)
        found_0_7 = true;
      if (std::abs(x0 - 7.0) < 1e-6 && std::abs(x1 - 20.0) < 1e-6)
        found_7_20 = true;
    }
  }
  EXPECT_TRUE(found_0_7);
  EXPECT_TRUE(found_7_20);
}

TEST_F(Sketch_test, AddNode_off_edge_adds_node_only)
{
  Headless_guard guard(view());

  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  Sketch_access::add_edge_(sketch, gp_Pnt2d(0.0, 0.0), gp_Pnt2d(20.0, 0.0));
  Sketch_access::update_faces_(sketch);

  const size_t nodes_before = sketch.get_nodes().size();

  gui().set_mode(Mode::Sketch_add_node);
  // Far enough from y=0 that headless snap radius (~100 plane units) does not pull onto the segment.
  ScreenCoords away(dvec2(10.0, 200.0));
  sketch.add_sketch_pt(away);

  EXPECT_EQ(Sketch_access::get_edge_count(sketch), 1u);
  EXPECT_EQ(sketch.get_nodes().size(), nodes_before + 1);
}

// Pick slightly off a segment in plane space; should snap onto the edge and split so the node stays snappable.
TEST_F(Sketch_test, AddNode_near_edge_snaps_onto_segment_and_splits)
{
  Headless_guard guard(view());

  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);
  Sketch_access::add_edge_(sketch, gp_Pnt2d(0.0, 0.0), gp_Pnt2d(20.0, 0.0));
  Sketch_access::update_faces_(sketch);

  gui().set_mode(Mode::Sketch_add_node);
  ScreenCoords near_edge(dvec2(7.0, 0.15));
  sketch.add_sketch_pt(near_edge);

  EXPECT_EQ(Sketch_access::get_edge_count(sketch), 2u)
      << "Near-miss pick should snap to segment and replace one edge with two";

  bool found_at_seven = false;
  for (size_t i = 0; i < sketch.get_nodes().size(); ++i)
  {
    if (sketch.get_nodes()[i].deleted)
      continue;

    const gp_Pnt2d& p = sketch.get_nodes()[i];
    if (std::abs(p.X() - 7.0) < 1e-5 && std::abs(p.Y()) < 1e-5)
    {
      found_at_seven = true;
      break;
    }
  }
  EXPECT_TRUE(found_at_seven);
}

TEST_F(Sketch_test, AddNode_distance_enter_undo)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  gui().set_mode(Mode::Sketch_add_node);
  sketch.add_sketch_pt(ScreenCoords(dvec2(0.0, 0.0)));
  sketch.sketch_pt_move(ScreenCoords(dvec2(10.0, 0.0)));

  Sketch_access::set_entered_edge_len(sketch, gp_Dir2d(1.0, 0.0), 10.0);
  sketch.on_enter();

  EXPECT_EQ(Sketch_access::count_permanent_nodes(sketch), 2u) << "Origin plus distance-placed node";

  EXPECT_GT(view().undo_stack_size(), 0u);
  EXPECT_TRUE(view().undo());
  EXPECT_EQ(Sketch_access::count_permanent_nodes(sketch), 1u) << "Undo should remove the distance-placed node";

  EXPECT_TRUE(view().redo());
  EXPECT_EQ(Sketch_access::count_permanent_nodes(sketch), 2u) << "Redo should restore the distance-placed node";
}

TEST_F(Sketch_test, AddNode_anchor_then_click_undo)
{
  Headless_guard guard(view());

  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  gui().set_mode(Mode::Sketch_add_node);
  sketch.add_sketch_pt(ScreenCoords(dvec2(0.0, 0.0)));
  sketch.add_sketch_pt(ScreenCoords(dvec2(10.0, 200.0)));

  EXPECT_EQ(Sketch_access::count_permanent_nodes(sketch), 2u) << "Origin plus click-placed node";

  EXPECT_GT(view().undo_stack_size(), 0u);
  EXPECT_TRUE(view().undo());
  EXPECT_EQ(Sketch_access::count_permanent_nodes(sketch), 1u) << "Undo should remove the click-placed node";

  EXPECT_TRUE(view().redo());
  EXPECT_EQ(Sketch_access::count_permanent_nodes(sketch), 2u) << "Redo should restore the click-placed node";
}

TEST_F(Sketch_test, DimensionTool_picks_sketch_origin)
{
  Headless_guard guard(view());

  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  const std::optional<size_t> origin_pick = sketch.get_nodes().try_pick_existing_node(ScreenCoords(dvec2(0.0, 0.0)));
  ASSERT_TRUE(origin_pick.has_value());
  EXPECT_TRUE(sketch.get_nodes()[*origin_pick].origin);
}
