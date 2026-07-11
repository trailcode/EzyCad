#include "sketch_test_fixture.h"

#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <numbers>

#include "sketch_edge.h"
#include "sketch_json.h"
#include "sketch_nodes.h"
#include "sketch_op_recorder.h"
#include "utl_geom.h"
#include "utl_occt.h"

using namespace glm;

TEST_F(Sketch_test, Undo_single_arc_via_recorder)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  const gp_Pnt2d start(0.0, 0.0);
  const gp_Pnt2d end(10.0, 0.0);
  const gp_Pnt2d arc_mid(5.0, 5.0);

  // Arc Segment tool: recorder starts on the third click, after all pick nodes exist.
  gui().set_mode(Mode::Sketch_add_seg_circle_arc);
  sketch.add_sketch_pt(ScreenCoords(dvec2(start.X(), start.Y())));
  sketch.add_sketch_pt(ScreenCoords(dvec2(end.X(), end.Y())));
  sketch.add_sketch_pt(ScreenCoords(dvec2(arc_mid.X(), arc_mid.Y())));

  EXPECT_EQ(Sketch_access::get_arc_internal_edge_count(sketch), 1);

  EXPECT_TRUE(view().undo());
  EXPECT_EQ(Sketch_access::get_arc_internal_edge_count(sketch), 0) << "Undo should remove the arc edge";

  EXPECT_TRUE(view().redo());
  EXPECT_EQ(Sketch_access::get_arc_internal_edge_count(sketch), 1) << "Redo should restore the arc";
}

TEST_F(Sketch_test, Undo_circle_via_recorder)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  gui().set_mode(Mode::Sketch_add_circle);
  sketch.add_sketch_pt(ScreenCoords(dvec2(0.0, 0.0)));
  sketch.add_sketch_pt(ScreenCoords(dvec2(5.0, 0.0)));
  sketch.finalize_elm();

  const size_t edges_after_create = Sketch_access::get_arc_internal_edge_count(sketch);
  const size_t perm_after_create  = Sketch_access::count_permanent_nodes(sketch);
  EXPECT_EQ(edges_after_create, 2u);

  EXPECT_TRUE(view().undo());
  EXPECT_EQ(Sketch_access::get_arc_internal_edge_count(sketch), 0u) << "Undo should remove both semicircles (two arc edges)";

  EXPECT_TRUE(view().redo());
  EXPECT_EQ(Sketch_access::get_arc_internal_edge_count(sketch), edges_after_create) << "Redo should restore the circle";
  EXPECT_EQ(Sketch_access::count_permanent_nodes(sketch), perm_after_create) << "Redo should not add extra permanent nodes";
}

TEST_F(Sketch_test, Undo_two_crossing_edges_via_finalize)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  {
    Sketch_op_recorder rec(view(), sketch);
    Sketch_access::add_edge_(sketch, gp_Pnt2d(0.0, 0.0), gp_Pnt2d(10.0, 0.0), rec);
    rec.commit();
  }

  {
    Sketch_op_recorder rec(view(), sketch);
    Sketch_access::add_edge_(sketch, gp_Pnt2d(3.0, -2.0), gp_Pnt2d(3.0, 4.0), rec);
    rec.commit();
  }

  EXPECT_EQ(Sketch_access::get_linear_edge_count(sketch), 4);

  EXPECT_TRUE(view().undo());
  EXPECT_EQ(Sketch_access::get_linear_edge_count(sketch), 1)
      << "Undo of the second edge should remove it and restore the unsplit first edge";

  EXPECT_TRUE(view().undo());
  EXPECT_EQ(Sketch_access::get_linear_edge_count(sketch), 0) << "Undo of the first edge should remove it";
}

TEST_F(Sketch_test, Redo_two_crossing_edges_keeps_intersection_node_non_permanent)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  auto find_live_node_at = [](Sketch& s, const gp_Pnt2d& pt) -> std::optional<size_t>
  {
    for (size_t i = 0, n = s.get_nodes().size(); i < n; ++i)
    {
      const auto& node = s.get_nodes()[i];
      if (node.deleted)
        continue;

      if (node.SquareDistance(pt) <= Precision::SquareConfusion())
        return i;
    }

    return std::nullopt;
  };

  {
    Sketch_op_recorder rec(view(), sketch);
    Sketch_access::add_edge_(sketch, gp_Pnt2d(0.0, 0.0), gp_Pnt2d(10.0, 0.0), rec);
    rec.commit();
  }

  {
    Sketch_op_recorder rec(view(), sketch);
    Sketch_access::add_edge_(sketch, gp_Pnt2d(3.0, -2.0), gp_Pnt2d(3.0, 4.0), rec);
    rec.commit();
  }

  EXPECT_EQ(Sketch_access::get_linear_edge_count(sketch), 4);
  const size_t perm_after_create = Sketch_access::count_permanent_nodes(sketch);

  const std::optional<size_t> intersection_before = find_live_node_at(sketch, gp_Pnt2d(3.0, 0.0));
  ASSERT_TRUE(intersection_before.has_value()) << "Crossing should create an interior node";
  EXPECT_FALSE(sketch.get_nodes()[*intersection_before].permanent) << "Auto-split intersection nodes should not be permanent";

  EXPECT_TRUE(view().undo());
  EXPECT_TRUE(view().undo());
  EXPECT_TRUE(view().redo());
  EXPECT_TRUE(view().redo());

  EXPECT_EQ(Sketch_access::get_linear_edge_count(sketch), 4);
  EXPECT_EQ(Sketch_access::count_permanent_nodes(sketch), perm_after_create) << "Redo should not add permanent nodes";

  const std::optional<size_t> intersection_after = find_live_node_at(sketch, gp_Pnt2d(3.0, 0.0));
  ASSERT_TRUE(intersection_after.has_value()) << "Crossing intersection should be restored";
  EXPECT_FALSE(sketch.get_nodes()[*intersection_after].permanent) << "Redo must not promote the intersection node to permanent";
}

TEST_F(Sketch_test, Undo_extrude_snapshot_keeps_single_sketch)
{
  Sketch::Sketch_ptr sk    = view().curr_sketch_shared();
  const size_t       sk_id = sk->get_id();

  {
    Sketch_op_recorder            rec(view(), *sk);
    const std::array<gp_Pnt2d, 4> corners = square_corners(gp_Pnt2d(0.0, 0.0), gp_Pnt2d(10.0, 0.0));
    Sketch_access::add_edge_(*sk, corners[0], corners[1], rec);
    Sketch_access::add_edge_(*sk, corners[1], corners[2], rec);
    Sketch_access::add_edge_(*sk, corners[2], corners[3], rec);
    Sketch_access::add_edge_(*sk, corners[3], corners[0], rec);
    rec.commit();
  }

  EXPECT_EQ(sk->edge_count(), 4u);
  EXPECT_EQ(view().get_sketches().size(), 1u);

  const nlohmann::json pre_extrude = nlohmann::json::parse(view().to_json());
  ASSERT_EQ(pre_extrude["sketches"].size(), 1u);

  view().push_undo_snapshot();

  EXPECT_TRUE(view().undo());
  EXPECT_EQ(view().get_sketches().size(), 1u) << "JSON undo after extrude must not add a sketch";

  Sketch::Sketch_ptr restored;
  for (const Sketch::Sketch_ptr& s : view().get_sketches())
    if (s->get_id() == sk_id)
      restored = s;

  ASSERT_TRUE(restored) << "Restored sketch should keep the same stable id";
  EXPECT_EQ(restored->edge_count(), 4u);

  EXPECT_TRUE(view().undo());
  EXPECT_EQ(restored->edge_count(), 0u) << "Second undo should remove the square from the same sketch";
}

TEST_F(Sketch_test, Undo_delta_resolves_by_sketch_id_when_names_duplicate)
{
  Sketch::Sketch_ptr sk1 = view().curr_sketch_shared();
  Sketch::Sketch_ptr sk2 = std::make_shared<Sketch>("Other", view(), gp_Pln(gp::Origin(), gp::DX()));
  view().get_sketches().push_back(sk2);
  sk1->set_name("Dup");
  sk2->set_name("Dup");

  {
    Sketch_op_recorder rec(view(), *sk2);
    Sketch_access::add_edge_(*sk2, gp_Pnt2d(0.0, 0.0), gp_Pnt2d(5.0, 0.0), rec);
    rec.commit();
  }

  EXPECT_EQ(Sketch_access::get_linear_edge_count(*sk1), 0u);
  EXPECT_EQ(Sketch_access::get_linear_edge_count(*sk2), 1u);

  EXPECT_TRUE(view().undo());
  EXPECT_EQ(Sketch_access::get_linear_edge_count(*sk1), 0u);
  EXPECT_EQ(Sketch_access::get_linear_edge_count(*sk2), 0u)
      << "Undo must target the sketch that was edited, not the first same-named sketch";
}

TEST_F(Sketch_test, Undo_circle_axis_then_revolve_snapshot_three_undos)
{
  Sketch::Sketch_ptr sk    = view().curr_sketch_shared();
  const size_t       sk_id = sk->get_id();

  {
    Sketch_op_recorder            rec(view(), *sk);
    const gp_Pnt2d                center(0.0, 0.0);
    const gp_Pnt2d                edge_pt(5.0, 0.0);
    const std::array<gp_Pnt2d, 4> points = xy_stencil_pnts(center, edge_pt);
    Sketch_access::add_arc_circle_(*sk, points[0], points[2], points[1], rec);
    Sketch_access::add_arc_circle_(*sk, points[0], points[3], points[1], rec);
    rec.commit();
  }

  EXPECT_EQ(Sketch_access::get_arc_internal_edge_count(*sk), 2u);

  {
    Sketch_op_recorder rec(view(), *sk);
    const gp_Pnt2d     axis_a(0.0, -10.0);
    const gp_Pnt2d     axis_b(10.0, -10.0);
    Sketch_access::set_operation_axis_(*sk, axis_a, axis_b);
    rec.note_curr_operation_axis(axis_a, axis_b);
    rec.commit();
  }
  EXPECT_TRUE(sk->has_operation_axis());

  view().push_undo_snapshot();

  auto find_sk = [&]() -> Sketch::Sketch_ptr
  {
    for (const Sketch::Sketch_ptr& s : view().get_sketches())
      if (s->get_id() == sk_id)
        return s;

    return nullptr;
  };

  EXPECT_TRUE(view().undo());
  sk = find_sk();
  ASSERT_TRUE(sk);
  EXPECT_EQ(view().get_sketches().size(), 1u);
  EXPECT_TRUE(sk->has_operation_axis());
  EXPECT_EQ(Sketch_access::get_arc_internal_edge_count(*sk), 2u);

  EXPECT_TRUE(view().undo());
  sk = find_sk();
  ASSERT_TRUE(sk);
  EXPECT_FALSE(sk->has_operation_axis());
  EXPECT_EQ(Sketch_access::get_arc_internal_edge_count(*sk), 2u);

  EXPECT_TRUE(view().undo());
  sk = find_sk();
  ASSERT_TRUE(sk);
  EXPECT_EQ(Sketch_access::get_arc_internal_edge_count(*sk), 0u);
}

// Operation axis persistence in sketch JSON save/load.
TEST_F(Sketch_test, OperationAxis_JSON_RoundTrip)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  gui().set_mode(Mode::Sketch_operation_axis);

  const gp_Pnt2d axis_start(0.0, 0.0);
  const gp_Pnt2d axis_end(10.0, 3.0);
  sketch.add_sketch_pt(ScreenCoords(dvec2(axis_start.X(), axis_start.Y())));
  sketch.add_sketch_pt(ScreenCoords(dvec2(axis_end.X(), axis_end.Y())));
  ASSERT_TRUE(sketch.has_operation_axis());

  const nlohmann::json j = Sketch_json::to_json(sketch, view().asset_store());
  ASSERT_TRUE(j.contains("operation_axis"));
  ASSERT_TRUE(j["operation_axis"].is_array());
  EXPECT_EQ(j["operation_axis"].size(), 2u);
  EXPECT_NEAR(j["operation_axis"][0]["x"].get<double>(), axis_start.X(), 1e-8);
  EXPECT_NEAR(j["operation_axis"][0]["y"].get<double>(), axis_start.Y(), 1e-8);
  EXPECT_NEAR(j["operation_axis"][1]["x"].get<double>(), axis_end.X(), 1e-8);
  EXPECT_NEAR(j["operation_axis"][1]["y"].get<double>(), axis_end.Y(), 1e-8);

  const std::shared_ptr<Sketch> loaded = Sketch_json::from_json(view(), j);
  ASSERT_TRUE(loaded->has_operation_axis());

  const nlohmann::json j2 = Sketch_json::to_json(*loaded, view().asset_store());
  ASSERT_TRUE(j2.contains("operation_axis"));
  EXPECT_NEAR(j2["operation_axis"][0]["x"].get<double>(), axis_start.X(), 1e-8);
  EXPECT_NEAR(j2["operation_axis"][0]["y"].get<double>(), axis_start.Y(), 1e-8);
  EXPECT_NEAR(j2["operation_axis"][1]["x"].get<double>(), axis_end.X(), 1e-8);
  EXPECT_NEAR(j2["operation_axis"][1]["y"].get<double>(), axis_end.Y(), 1e-8);
}

// Mirror with no selection: error message and Sketch_op_recorder::cancel() via headless GUI
// (operation axis clicks + Options panel Mirror button).

// Mirror with no selection: error message and Sketch_op_recorder::cancel() via headless GUI
// (operation axis clicks + Options panel Mirror button).
TEST_F(Sketch_test, MirrorSelectedEdges_NoEdgesSelected)
{
  Headless_guard guard(view());

  Sketch& sketch = view().curr_sketch();

  gui().set_mode(Mode::Sketch_operation_axis);
  GUI_access::sketch_left_click(gui(), ScreenCoords(dvec2(0.0, 0.0)));
  GUI_access::sketch_left_click(gui(), ScreenCoords(dvec2(10.0, 0.0)));

  EXPECT_TRUE(sketch.has_operation_axis()) << "Operation axis should be created";

  gui().show_message("");
  const size_t undo_steps_before = view().undo_stack_size();

  GUI_access::mirror_selected_edges(gui());

  EXPECT_EQ(GUI_access::get_message(gui()), Sketch::ERROR_NO_EDGES_SELECTED)
      << "Error message should be displayed when no edges are selected";
  EXPECT_EQ(view().undo_stack_size(), undo_steps_before) << "Mirror with no selection must not push an undo step";
}

// Positive test: mirror a simple straight edge across a horizontal axis.
// The length of the operation axis itself has no effect; only its direction and position matter.

// Positive test: mirror a simple straight edge across a horizontal axis.
// The length of the operation axis itself has no effect; only its direction and position matter.
TEST_F(Sketch_test, MirrorSelectedEdges_SimpleStraightEdge)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  // Activate operation axis mode and define a horizontal axis (y=0)
  gui().set_mode(Mode::Sketch_operation_axis);

  gp_Pnt2d axis_p1(0.0, 0.0);
  gp_Pnt2d axis_p2(5.0, 0.0); // length is arbitrary / visual only
  sketch.add_sketch_pt(ScreenCoords(dvec2(axis_p1.X(), axis_p1.Y())));
  sketch.add_sketch_pt(ScreenCoords(dvec2(axis_p2.X(), axis_p2.Y())));

  EXPECT_TRUE(sketch.has_operation_axis()) << "Operation axis must be defined for mirror";

  // Add a horizontal edge above the axis to be mirrored
  gp_Pnt2d e1(1.0, 2.0);
  gp_Pnt2d e2(4.0, 2.0);
  Sketch_access::add_edge_(sketch, e1, e2);

  // Find the shp of the edge we just added (last one with b node)
  AIS_Shape_ptr edge_to_mirror;
  for (auto rit = Sketch_access::get_edges(sketch).rbegin(); rit != Sketch_access::get_edges(sketch).rend(); ++rit)
    if (rit->node_idx_b.has_value())
    {
      edge_to_mirror = rit->shp;
      break;
    }
  
  ASSERT_FALSE(edge_to_mirror.IsNull()) << "Expected to find a selectable edge shp";

  // Simulate selection via the interactive context (mirror/revolve use get_selected_edges_ which reads from view selection)
  auto& cctx = view().ctx();
  cctx.ClearSelected(true);
  cctx.AddOrRemoveSelected(edge_to_mirror, true);

  size_t before = Sketch_access::get_edge_count(sketch);

  // Execute mirror
  sketch.mirror_selected_edges();

  size_t after = Sketch_access::get_edge_count(sketch);
  EXPECT_EQ(after, before + 1) << "Exactly one mirrored edge should be added";

  // Verify a mirrored edge now exists at y = -2 with matching x coords
  bool found_mirrored = false;
  for (const auto& e : Sketch_access::get_edges(sketch))
  {
    if (!e.node_idx_b.has_value())
      continue;

    const gp_Pnt2d& na = sketch.get_nodes()[e.node_idx_a];
    const gp_Pnt2d& nb = sketch.get_nodes()[*e.node_idx_b];
    if (std::abs(na.Y() + 2.0) < Precision::Confusion() && std::abs(nb.Y() + 2.0) < Precision::Confusion() &&
        std::abs(na.X() - 1.0) < Precision::Confusion() && std::abs(nb.X() - 4.0) < Precision::Confusion())
    {
      found_mirrored = true;
      break;
    }
  }
  EXPECT_TRUE(found_mirrored) << "Mirrored edge (1,-2) to (4,-2) should have been created";
}

// Test mirroring works across a vertical axis as well
TEST_F(Sketch_test, MirrorSelectedEdges_VerticalAxis)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  gui().set_mode(Mode::Sketch_operation_axis);

  // Vertical axis at x=0
  sketch.add_sketch_pt(ScreenCoords(dvec2(0.0, 0.0)));
  sketch.add_sketch_pt(ScreenCoords(dvec2(0.0, 5.0)));

  // Edge to the right of the axis
  gp_Pnt2d e1(2.0, 1.0);
  gp_Pnt2d e2(2.0, 3.0);
  Sketch_access::add_edge_(sketch, e1, e2);

  // Select it
  AIS_Shape_ptr shp;
  for (auto rit = Sketch_access::get_edges(sketch).rbegin(); rit != Sketch_access::get_edges(sketch).rend(); ++rit)
    if (rit->node_idx_b.has_value())
    {
      shp = rit->shp;
      break;
    }
  
  ASSERT_FALSE(shp.IsNull());

  auto& cctx = view().ctx();
  cctx.ClearSelected(true);
  cctx.AddOrRemoveSelected(shp, true);

  size_t before = Sketch_access::get_edge_count(sketch);

  sketch.mirror_selected_edges();

  size_t after = Sketch_access::get_edge_count(sketch);

  EXPECT_EQ(after, before + 1);

  // Expect mirrored edge at x=-2
  bool found = false;
  for (const auto& e : Sketch_access::get_edges(sketch))
  {
    if (!e.node_idx_b.has_value())
      continue;

    const gp_Pnt2d& na = sketch.get_nodes()[e.node_idx_a];
    const gp_Pnt2d& nb = sketch.get_nodes()[*e.node_idx_b];
    if (std::abs(na.X() + 2.0) < Precision::Confusion() && std::abs(nb.X() + 2.0) < Precision::Confusion() &&
        std::abs(na.Y() - 1.0) < Precision::Confusion() && std::abs(nb.Y() - 3.0) < Precision::Confusion())
    {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found) << "Mirrored edge should appear on the left of vertical axis";
}

// Test that mirroring an arc (circle arc pair) works
TEST_F(Sketch_test, MirrorSelectedEdges_Arc)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  gui().set_mode(Mode::Sketch_operation_axis);

  // Horizontal axis
  sketch.add_sketch_pt(ScreenCoords(dvec2(0.0, 0.0)));
  sketch.add_sketch_pt(ScreenCoords(dvec2(10.0, 0.0)));

  // Add a simple arc above the axis (using three points)
  // Arc from (-1,1) through (0,2) to (1,1) — a bump above x axis
  gp_Pnt2d a(-1.0, 1.0);
  gp_Pnt2d b(0.0, 2.0);
  gp_Pnt2d c(1.0, 1.0);
  Sketch_access::add_arc_circle_(sketch, a, b, c);

  // For arcs, the shp is shared; select the shp of one of the arc edges
  AIS_Shape_ptr arc_shp;
  for (const auto& e : Sketch_access::get_edges(sketch))
    if (sketch_edge_is_arc(e))
    {
      arc_shp = e.shp;
      break;
    }
  
  ASSERT_FALSE(arc_shp.IsNull()) << "Arc should have been added with a shp";

  auto& cctx = view().ctx();
  cctx.ClearSelected(true);
  cctx.AddOrRemoveSelected(arc_shp, true);

  size_t before = Sketch_access::get_edge_count(sketch);

  sketch.mirror_selected_edges();

  // Mirroring an arc pair should add another arc pair below
  size_t after = Sketch_access::get_edge_count(sketch);

  // Expect +1 edge for the mirrored arc
  EXPECT_EQ(after, before + 1) << "Mirroring an arc should add one new arc edge";

  // Spot-check that a mirrored point exists below the axis (e.g. around y=-1 or so)
  bool found_symmetric = false;
  for (const auto& e : Sketch_access::get_edges(sketch))
  {
    if (!sketch_edge_is_arc(e))
      continue;

    const gp_Pnt2d& pa = sketch.get_nodes()[e.node_idx_a];
    if (std::abs(pa.Y() + 1.0) < 0.5) // rough, symmetric to +1
    {
      found_symmetric = true;
      break;
    }
  }
  EXPECT_TRUE(found_symmetric) << "A mirrored arc point should exist below the axis";
}

// Unit tests for the revolve tool (Sketch::revolve_selected).
// Revolve requires a defined operation axis + selected edges or faces.
// It returns a Shp_rslt (success or error). On success it produces a revolved 3D shape via BRepPrimAPI_MakeRevol.

// Error case: no edges/faces selected (after axis is set)

// Error case: no edges/faces selected (after axis is set)
TEST_F(Sketch_test, RevolveSelected_NoSelection)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  gui().set_mode(Mode::Sketch_operation_axis);

  // Define a simple horizontal axis
  sketch.add_sketch_pt(ScreenCoords(dvec2(0.0, 0.0)));
  sketch.add_sketch_pt(ScreenCoords(dvec2(5.0, 0.0)));

  EXPECT_TRUE(sketch.has_operation_axis());

  // Call without any selection
  Shp_rslt res = sketch.revolve_selected(2 * std::numbers::pi);

  EXPECT_FALSE(res.has_value());
  EXPECT_EQ(res.message(), "No selected faces or edges.");
}

// Basic success case: revolve a straight edge profile around the axis (360 deg full revolution).
// Uses edge selection (common for profile-based revolution).

// Basic success case: revolve a straight edge profile around the axis (360 deg full revolution).
// Uses edge selection (common for profile-based revolution).
TEST_F(Sketch_test, RevolveSelected_SimpleEdgeProfile)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  gui().set_mode(Mode::Sketch_operation_axis);

  // Horizontal axis at y=0
  sketch.add_sketch_pt(ScreenCoords(dvec2(0.0, 0.0)));
  sketch.add_sketch_pt(ScreenCoords(dvec2(10.0, 0.0)));

  // Add a profile edge offset from the axis (a line segment parallel-ish, at x~2, y from 1 to 2)
  // Revolving this around the x-axis will create a surface of revolution.
  gp_Pnt2d p1(2.0, 1.0);
  gp_Pnt2d p2(2.0, 2.0);
  Sketch_access::add_edge_(sketch, p1, p2);

  // Select the edge shp (same mechanism used by mirror tests and the real UI selection)
  AIS_Shape_ptr edge_shp;
  for (auto rit = Sketch_access::get_edges(sketch).rbegin(); rit != Sketch_access::get_edges(sketch).rend(); ++rit)
    if (rit->node_idx_b.has_value())
    {
      edge_shp = rit->shp;
      break;
    }
  
  ASSERT_FALSE(edge_shp.IsNull());

  auto& cctx = view().ctx();
  cctx.ClearSelected(true);
  cctx.AddOrRemoveSelected(edge_shp, true);

  Shp_rslt res = sketch.revolve_selected(2 * std::numbers::pi);

  EXPECT_TRUE(res.has_value()) << "Revolve should succeed with a valid axis + selected edge. Message: " << res.message();
  ASSERT_TRUE(res.has_value());

  const TopoDS_Shape& actual = (*res)->Shape();
  EXPECT_FALSE(actual.IsNull()) << "Revolved shape must not be null";

  // === Stronger "oracle" test: rebuild the exact same revolve using low-level OCCT ===
  // This verifies that Sketch::revolve_selected produces the *identical* geometry
  // as a direct BRepPrimAPI_MakeRevol call with the extracted profile + axis.

  // 1. Build the profile compound (same edges the sketch would have selected)
  TopoDS_Compound profile;
  BRep_Builder    bld;
  bld.MakeCompound(profile);
  bld.Add(profile, edge_shp->Shape()); // the edge we selected above

  // 2. Recreate the axis using the exact same two points we used to define the operation axis.
  //    (Only direction + a point on the line matter; length is irrelevant.)
  gp_Pnt axA(0.0, 0.0, 0.0);
  gp_Pnt axB(10.0, 0.0, 0.0);
  gp_Dir dir((axB.XYZ() - axA.XYZ()).Normalized());
  gp_Ax1 revolAxis(axA, dir);

  BRepPrimAPI_MakeRevol expectedMaker(profile, revolAxis, 2 * std::numbers::pi);
  TopoDS_Shape          expected = try_make_solid(expectedMaker.Shape());

  EXPECT_FALSE(expected.IsNull());

  // Stronger oracle comparison using mass properties + bounding box.
  // This verifies that the geometry produced by the Sketch revolve wrapper
  // is the same as a direct low-level BRepPrimAPI_MakeRevol call.
  GProp_GProps gActual, gExpected;
  BRepGProp::VolumeProperties(actual, gActual);
  BRepGProp::VolumeProperties(expected, gExpected);

  EXPECT_NEAR(gActual.Mass(), gExpected.Mass(), 1e-8) << "Revolved volume should match direct MakeRevol";

  Bnd_Box boxA, boxE;
  BRepBndLib::Add(actual, boxA);
  BRepBndLib::Add(expected, boxE);

  double axmin, aymin, azmin, axmax, aymax, azmax;
  double exmin, eymin, ezmin, exmax, eymax, ezmax;
  boxA.Get(axmin, aymin, azmin, axmax, aymax, azmax);
  boxE.Get(exmin, eymin, ezmin, exmax, eymax, ezmax);

  EXPECT_NEAR(axmin, exmin, 1e-8);
  EXPECT_NEAR(aymin, eymin, 1e-8);
  EXPECT_NEAR(azmin, ezmin, 1e-8);
  EXPECT_NEAR(axmax, exmax, 1e-8);
  EXPECT_NEAR(aymax, eymax, 1e-8);
  EXPECT_NEAR(azmax, ezmax, 1e-8);

  // It will typically be a Face (surface of revolution) for an open edge profile.
  // We don't assert Solid here because that usually requires a closed face profile.
}

// Revolve a closed edge profile (rectangle) by selecting its boundary edges.
// This exercises the selected_edges path in revolve_selected (multiple edges in compound).
// Revolving a closed profile 360° around an external axis produces a solid of revolution.

// Revolve a closed edge profile (rectangle) by selecting its boundary edges.
// This exercises the selected_edges path in revolve_selected (multiple edges in compound).
// Revolving a closed profile 360° around an external axis produces a solid of revolution.
TEST_F(Sketch_test, RevolveSelected_ClosedEdgeProfile)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  gui().set_mode(Mode::Sketch_operation_axis);

  // Axis
  sketch.add_sketch_pt(ScreenCoords(dvec2(0.0, 0.0)));
  sketch.add_sketch_pt(ScreenCoords(dvec2(10.0, 0.0)));

  // Simple closed rectangular profile offset from the axis (will form a face too, but we select edges).
  gp_Pnt2d p0(1.0, 1.0);
  gp_Pnt2d p1(3.0, 1.0);
  gp_Pnt2d p2(3.0, 2.0);
  gp_Pnt2d p3(1.0, 2.0);

  Sketch_access::add_edge_(sketch, p0, p1);
  Sketch_access::add_edge_(sketch, p1, p2);
  Sketch_access::add_edge_(sketch, p2, p3);
  Sketch_access::add_edge_(sketch, p3, p0);

  // Select all four profile edges (the revolve code will compound their shapes)
  auto& cctx = view().ctx();
  cctx.ClearSelected(true);

  size_t selected_count = 0;
  for (const auto& e : Sketch_access::get_edges(sketch))
    if (e.node_idx_b.has_value())
    {
      // Only select the ones we just added for the profile (simple heuristic by x/y range)
      const gp_Pnt2d& na = sketch.get_nodes()[e.node_idx_a];
      const gp_Pnt2d& nb = sketch.get_nodes()[*e.node_idx_b];
      if (na.X() >= 0.5 && na.X() <= 3.5 && nb.X() >= 0.5 && nb.X() <= 3.5)
      {
        cctx.AddOrRemoveSelected(e.shp, true);
        ++selected_count;
      }
    }
  
  EXPECT_GE(selected_count, 2) << "Should have selected at least a couple of profile edges";

  Shp_rslt res = sketch.revolve_selected(2 * std::numbers::pi);

  EXPECT_TRUE(res.has_value()) << "Revolve of closed edge profile should succeed. Message: " << res.message();
  ASSERT_TRUE(res.has_value());

  const TopoDS_Shape& actual = (*res)->Shape();
  EXPECT_FALSE(actual.IsNull()) << "Revolved shape must not be null";

  // === Oracle comparison: rebuild the identical revolve using the same inputs ===
  TopoDS_Compound profile;
  BRep_Builder    bld;
  bld.MakeCompound(profile);

  // Collect exactly the profile edges we added for this test
  for (const auto& e : Sketch_access::get_edges(sketch))
    if (e.node_idx_b.has_value())
    {
      const gp_Pnt2d& na = sketch.get_nodes()[e.node_idx_a];
      const gp_Pnt2d& nb = sketch.get_nodes()[*e.node_idx_b];
      if (na.X() >= 0.5 && na.X() <= 3.5 && nb.X() >= 0.5 && nb.X() <= 3.5)
        bld.Add(profile, e.shp->Shape());
    }

  // Reconstruct the axis from the same points we used when creating the operation axis
  gp_Pnt axA(0.0, 0.0, 0.0);
  gp_Pnt axB(10.0, 0.0, 0.0);
  gp_Dir dir((axB.XYZ() - axA.XYZ()).Normalized());
  gp_Ax1 revolAxis(axA, dir);

  BRepPrimAPI_MakeRevol expectedMaker(profile, revolAxis, 2 * std::numbers::pi);
  TopoDS_Shape          expected = try_make_solid(expectedMaker.Shape());

  EXPECT_FALSE(expected.IsNull());

  GProp_GProps gActual, gExpected;
  BRepGProp::VolumeProperties(actual, gActual);
  BRepGProp::VolumeProperties(expected, gExpected);

  EXPECT_NEAR(gActual.Mass(), gExpected.Mass(), 1e-8) << "Revolved volume should match direct MakeRevol";

  Bnd_Box boxA, boxE;
  BRepBndLib::Add(actual, boxA);
  BRepBndLib::Add(expected, boxE);

  double axmin, aymin, azmin, axmax, aymax, azmax;
  double exmin, eymin, ezmin, exmax, eymax, ezmax;
  boxA.Get(axmin, aymin, azmin, axmax, aymax, azmax);
  boxE.Get(exmin, eymin, ezmin, exmax, eymax, ezmax);

  EXPECT_NEAR(axmin, exmin, 1e-8);
  EXPECT_NEAR(aymin, eymin, 1e-8);
  EXPECT_NEAR(azmin, ezmin, 1e-8);
  EXPECT_NEAR(axmax, exmax, 1e-8);
  EXPECT_NEAR(aymax, eymax, 1e-8);
  EXPECT_NEAR(azmax, ezmax, 1e-8);

  // For a closed profile revolved 360°, we expect a solid of revolution.
  EXPECT_EQ(actual.ShapeType(), TopAbs_SOLID) << "Closed edge profile revolved 360 deg should be a solid";
}

// New file clears undo/redo stacks
TEST_F(Sketch_test, NewFile_clears_undo_stacks)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  gui().set_mode(Mode::Sketch_add_node);
  sketch.add_sketch_pt(ScreenCoords(dvec2(10.0, 10.0)));
  ASSERT_GT(view().undo_stack_size(), 0u);

  view().new_file();
  EXPECT_EQ(view().undo_stack_size(), 0u);
  EXPECT_EQ(view().redo_stack_size(), 0u);
}
