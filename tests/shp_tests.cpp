#include "skt_test_fixture.h"

#include <AIS_InteractiveContext.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Ax1.hxx>
#include <gp_Pln.hxx>
#include <gp_Trsf.hxx>
#include <numbers>

#include "shp.h"
#include "shp_create.h"
#include "shp_info.h"
#include "shp_cross_section.h"
#include "skt_op_recorder.h"
#include "utl.h"

namespace
{
double volume_of(const TopoDS_Shape& shape)
{
  GProp_GProps props;
  BRepGProp::VolumeProperties(shape, props);
  return props.Mass();
}

void get_bbox(const TopoDS_Shape& shape, double& xmin, double& ymin, double& zmin, double& xmax, double& ymax, double& zmax)
{
  Bnd_Box bbox;
  BRepBndLib::Add(shape, bbox);
  ASSERT_FALSE(bbox.IsVoid());
  bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
}

bool contains_solid_like(const TopoDS_Shape& shape)
{
  return !shape.IsNull() && (shape.ShapeType() == TopAbs_SOLID || TopExp_Explorer(shape, TopAbs_SOLID).More());
}

std::string line_value(const std::vector<shp_info::Line>& lines, const char* label)
{
  for (const auto& line : lines)
    if (line.label == label)
      return line.value;

  return {};
}

void select_shapes(Occt_view& view, const std::vector<Shp_ptr>& shapes)
{
  AIS_InteractiveContext& cctx = view.ctx();
  cctx.ClearSelected(true);
  for (const Shp_ptr& shp : shapes)
    cctx.AddOrRemoveSelected(shp, true);
}
} // namespace

// Headless Occt_view fixture shared with sketch tests.
class Shp_test : public Sketch_test
{
};

// ---------------------------------------------------------------------------
// shp_create primitives (no viewer)
// ---------------------------------------------------------------------------

TEST(Shp_create, Box_solid_volume_and_origin)
{
  const TopoDS_Shape box = shp_create::create_box(1.0, 2.0, 3.0, 4.0, 5.0, 6.0);
  ASSERT_FALSE(box.IsNull());
  EXPECT_EQ(box.ShapeType(), TopAbs_SOLID);
  EXPECT_TRUE(BRepCheck_Analyzer(box).IsValid());
  EXPECT_NEAR(volume_of(box), 4.0 * 5.0 * 6.0, 1e-6);

  double xmin, ymin, zmin, xmax, ymax, zmax;
  get_bbox(box, xmin, ymin, zmin, xmax, ymax, zmax);
  EXPECT_NEAR(xmin, 1.0, 1e-6);
  EXPECT_NEAR(ymin, 2.0, 1e-6);
  EXPECT_NEAR(zmin, 3.0, 1e-6);
  EXPECT_NEAR(xmax, 5.0, 1e-6);
  EXPECT_NEAR(ymax, 7.0, 1e-6);
  EXPECT_NEAR(zmax, 9.0, 1e-6);
}

TEST(Shp_create, Sphere_volume_centered_at_origin)
{
  const double       r      = 2.0;
  const TopoDS_Shape sphere = shp_create::create_sphere(r);
  ASSERT_FALSE(sphere.IsNull());
  EXPECT_EQ(sphere.ShapeType(), TopAbs_SOLID);
  EXPECT_TRUE(BRepCheck_Analyzer(sphere).IsValid());
  EXPECT_NEAR(volume_of(sphere), (4.0 / 3.0) * std::numbers::pi * r * r * r, 1e-4);

  double xmin, ymin, zmin, xmax, ymax, zmax;
  get_bbox(sphere, xmin, ymin, zmin, xmax, ymax, zmax);
  EXPECT_NEAR(xmin, -r, 1e-6);
  EXPECT_NEAR(ymin, -r, 1e-6);
  EXPECT_NEAR(zmin, -r, 1e-6);
  EXPECT_NEAR(xmax, r, 1e-6);
  EXPECT_NEAR(ymax, r, 1e-6);
  EXPECT_NEAR(zmax, r, 1e-6);
}

TEST(Shp_create, Cylinder_volume_centered_on_z)
{
  const double       radius = 1.5;
  const double       height = 4.0;
  const TopoDS_Shape cyl    = shp_create::create_cylinder(radius, height);
  ASSERT_FALSE(cyl.IsNull());
  EXPECT_EQ(cyl.ShapeType(), TopAbs_SOLID);
  EXPECT_TRUE(BRepCheck_Analyzer(cyl).IsValid());
  EXPECT_NEAR(volume_of(cyl), std::numbers::pi * radius * radius * height, 1e-4);

  double xmin, ymin, zmin, xmax, ymax, zmax;
  get_bbox(cyl, xmin, ymin, zmin, xmax, ymax, zmax);
  EXPECT_NEAR(zmin, -height / 2.0, 1e-6);
  EXPECT_NEAR(zmax, height / 2.0, 1e-6);
  EXPECT_NEAR(xmin, -radius, 1e-6);
  EXPECT_NEAR(xmax, radius, 1e-6);
}

TEST(Shp_create, Cone_volume_centered_on_z)
{
  const double       R1     = 3.0;
  const double       R2     = 1.0;
  const double       height = 6.0;
  const TopoDS_Shape cone   = shp_create::create_cone(R1, R2, height);
  ASSERT_FALSE(cone.IsNull());
  EXPECT_EQ(cone.ShapeType(), TopAbs_SOLID);
  EXPECT_TRUE(BRepCheck_Analyzer(cone).IsValid());

  // Truncated cone volume: (1/3) pi h (R1^2 + R1 R2 + R2^2)
  const double expected = (1.0 / 3.0) * std::numbers::pi * height * (R1 * R1 + R1 * R2 + R2 * R2);
  EXPECT_NEAR(volume_of(cone), expected, 1e-3);

  double xmin, ymin, zmin, xmax, ymax, zmax;
  get_bbox(cone, xmin, ymin, zmin, xmax, ymax, zmax);
  EXPECT_NEAR(zmin, -height / 2.0, 1e-6);
  EXPECT_NEAR(zmax, height / 2.0, 1e-6);
}

TEST(Shp_create, Torus_volume)
{
  const double       R1    = 5.0;
  const double       R2    = 1.0;
  const TopoDS_Shape torus = shp_create::create_torus(R1, R2);
  ASSERT_FALSE(torus.IsNull());
  EXPECT_EQ(torus.ShapeType(), TopAbs_SOLID);
  EXPECT_TRUE(BRepCheck_Analyzer(torus).IsValid());
  EXPECT_NEAR(volume_of(torus), 2.0 * std::numbers::pi * std::numbers::pi * R1 * R2 * R2, 1e-2);
}

TEST(Shp_create, Pyramid_solid_non_null)
{
  const double       side    = 4.0;
  const TopoDS_Shape pyramid = shp_create::create_pyramid(side);
  ASSERT_FALSE(pyramid.IsNull());
  EXPECT_EQ(pyramid.ShapeType(), TopAbs_SOLID);
  EXPECT_TRUE(BRepCheck_Analyzer(pyramid).IsValid());

  // Square pyramid volume = (1/3) * base_area * height; base side == height == side
  EXPECT_NEAR(volume_of(pyramid), (1.0 / 3.0) * side * side * side, 1e-3);

  double xmin, ymin, zmin, xmax, ymax, zmax;
  get_bbox(pyramid, xmin, ymin, zmin, xmax, ymax, zmax);
  EXPECT_NEAR(zmin, -side / 2.0, 1e-6);
  EXPECT_NEAR(zmax, side / 2.0, 1e-6);
}

// ---------------------------------------------------------------------------
// Cross-section geometry (no viewer)
// ---------------------------------------------------------------------------

TEST(Shp_cross_section, Box_midplane_is_four_lines)
{
  const TopoDS_Shape box   = shp_create::create_box(0, 0, 0, 4, 6, 8);
  const gp_Ax3       frame = gp_Ax3(gp_Pnt(2, 3, 4), gp::DZ(), gp::DX());

  const Result<Cross_section_geometry> result = cross_section_shape(box, frame, Cross_section_plane::XY, 0.0);
  ASSERT_TRUE(result.has_value()) << result.message();
  EXPECT_EQ((*result).edge_count, 4u);
  EXPECT_EQ((*result).line_count, 4u);
}

TEST(Shp_cross_section, Cylinder_midplane_contains_circle)
{
  const TopoDS_Shape cylinder = shp_create::create_cylinder(2.0, 6.0);
  const gp_Ax3       frame(gp::Origin(), gp::DZ(), gp::DX());

  const Result<Cross_section_geometry> result = cross_section_shape(cylinder, frame, Cross_section_plane::XY, 0.0);
  ASSERT_TRUE(result.has_value()) << result.message();
  EXPECT_GE((*result).edge_count, 1u);
  EXPECT_GE((*result).circle_count, 1u);
}

TEST(Shp_cross_section, Offset_outside_box_fails_closed)
{
  const TopoDS_Shape box   = shp_create::create_box(0, 0, 0, 4, 6, 8);
  const gp_Ax3       frame = gp_Ax3(gp_Pnt(2, 3, 4), gp::DZ(), gp::DX());

  const Result<Cross_section_geometry> result = cross_section_shape(box, frame, Cross_section_plane::XY, 5.0);
  EXPECT_FALSE(result.has_value());
}

TEST(Shp_cross_section, Compound_with_solid_is_supported)
{
  TopoDS_Compound compound;
  BRep_Builder    builder;
  builder.MakeCompound(compound);
  builder.Add(compound, shp_create::create_box(0, 0, 0, 4, 6, 8));

  const gp_Ax3                   frame(gp_Pnt(2, 3, 4), gp::DZ(), gp::DX());
  const Result<Cross_section_geometry> result = cross_section_shape(compound, frame, Cross_section_plane::XY, 0.0);
  ASSERT_TRUE(result.has_value()) << result.message();
  EXPECT_EQ((*result).line_count, 4u);
}

TEST(Shp_cross_section, Empty_compound_is_rejected)
{
  TopoDS_Compound compound;
  BRep_Builder().MakeCompound(compound);

  const Result<Cross_section_geometry> result = cross_section_shape(compound, gp_Ax3(), Cross_section_plane::XY, 0.0);
  EXPECT_FALSE(result.has_value());
}

TEST(Shp_cross_section, Face_only_compound_is_rejected)
{
  const TopoDS_Shape face = BRepBuilderAPI_MakeFace(gp_Pln(gp::Origin(), gp::DZ()), -1.0, 1.0, -1.0, 1.0).Face();
  TopoDS_Compound    compound;
  BRep_Builder       builder;
  builder.MakeCompound(compound);
  builder.Add(compound, face);

  const Result<Cross_section_geometry> result = cross_section_shape(compound, gp_Ax3(), Cross_section_plane::XY, 0.0);
  EXPECT_FALSE(result.has_value());
}

TEST(Shp_cross_section, Box_local_xz_and_yz_midplanes_are_four_lines)
{
  const TopoDS_Shape box   = shp_create::create_box(0, 0, 0, 4, 6, 8);
  const gp_Ax3       frame = gp_Ax3(gp_Pnt(2, 3, 4), gp::DZ(), gp::DX());

  const Result<Cross_section_geometry> xz = cross_section_shape(box, frame, Cross_section_plane::XZ, 0.0);
  ASSERT_TRUE(xz.has_value()) << xz.message();
  EXPECT_EQ((*xz).edge_count, 4u);
  EXPECT_EQ((*xz).line_count, 4u);

  const Result<Cross_section_geometry> yz = cross_section_shape(box, frame, Cross_section_plane::YZ, 0.0);
  ASSERT_TRUE(yz.has_value()) << yz.message();
  EXPECT_EQ((*yz).edge_count, 4u);
  EXPECT_EQ((*yz).line_count, 4u);
}

TEST(Shp_cross_section, Rotated_frame_midplane_is_four_lines)
{
  const TopoDS_Shape box = shp_create::create_box(0, 0, 0, 4, 6, 8);
  gp_Ax3             frame(gp_Pnt(2, 3, 4), gp::DZ(), gp::DX());
  gp_Trsf            rotate;
  rotate.SetRotation(gp_Ax1(frame.Location(), gp::DX()), std::numbers::pi / 2.0);
  frame.Transform(rotate);

  const Result<Cross_section_geometry> result = cross_section_shape(box, frame, Cross_section_plane::XY, 0.0);
  ASSERT_TRUE(result.has_value()) << result.message();
  EXPECT_EQ((*result).edge_count, 4u);
  EXPECT_EQ((*result).line_count, 4u);
}

TEST(Shp_cross_section, Fused_boxes_midplane_returns_extra_line_edges)
{
  const TopoDS_Shape a = shp_create::create_box(0, 0, 0, 10, 10, 10);
  const TopoDS_Shape b = shp_create::create_box(5, 0, 0, 10, 10, 10);
  BRepAlgoAPI_Fuse   fuse(a, b);
  fuse.Build();
  ASSERT_TRUE(fuse.IsDone());

  const gp_Ax3                   frame(gp_Pnt(7.5, 5, 5), gp::DZ(), gp::DX());
  const Result<Cross_section_geometry> result = cross_section_shape(fuse.Shape(), frame, Cross_section_plane::XY, 0.0);
  ASSERT_TRUE(result.has_value()) << result.message();
  // Ideal outer section is a 15x10 rectangle (4 lines), but BRepAlgoAPI_Section currently
  // returns 8 line edges here. Keep this expectation as discovery evidence for sketch import.
  EXPECT_EQ((*result).edge_count, 8u);
  EXPECT_EQ((*result).line_count, 8u);
  EXPECT_EQ((*result).other_curve_count, 0u);
}

TEST(Shp_cross_section, Shared_plane_cuts_two_separated_boxes)
{
  // Mimic multi-select: one plane through the combined bbox center must hit both solids.
  const TopoDS_Shape left  = shp_create::create_box(0, 0, 0, 4, 4, 4);
  const TopoDS_Shape right = shp_create::create_box(6, 0, 0, 4, 4, 4);
  const gp_Ax3       shared_frame(gp_Pnt(5, 2, 2), gp::DZ(), gp::DX());

  const Result<Cross_section_geometry> left_result  = cross_section_shape(left, shared_frame, Cross_section_plane::XY, 0.0);
  const Result<Cross_section_geometry> right_result = cross_section_shape(right, shared_frame, Cross_section_plane::XY, 0.0);
  ASSERT_TRUE(left_result.has_value()) << left_result.message();
  ASSERT_TRUE(right_result.has_value()) << right_result.message();
  EXPECT_EQ((*left_result).line_count, 4u);
  EXPECT_EQ((*right_result).line_count, 4u);
}

TEST_F(Shp_test, Cross_section_previews_on_mode_enter_with_selection)
{
  view().add_box(0, 0, 0, 10, 10, 10);
  select_shapes(view(), {view().get_shapes().back()});
  EXPECT_FALSE(view().shp_cross_section().has_preview());

  gui().set_mode(Mode::Shape_cross_section);

  EXPECT_TRUE(view().shp_cross_section().has_preview());
  EXPECT_EQ(gui().get_mode(), Mode::Shape_cross_section);
  EXPECT_FALSE(view().shp_cross_section().selection_stale());
}

TEST_F(Shp_test, Cross_section_selection_stale_after_selection_change)
{
  view().add_box(0, 0, 0, 10, 10, 10);
  view().add_box(20, 0, 0, 10, 10, 10);
  const std::vector<Shp_ptr> boxes(view().get_shapes().begin(), view().get_shapes().end());
  ASSERT_EQ(boxes.size(), 2u);

  select_shapes(view(), {boxes[0]});
  gui().set_mode(Mode::Shape_cross_section);
  ASSERT_TRUE(view().shp_cross_section().has_preview());
  EXPECT_FALSE(view().shp_cross_section().selection_stale());
  EXPECT_FALSE(view().shp_cross_section().preview_inputs_stale());

  select_shapes(view(), {boxes[1]});
  EXPECT_TRUE(view().shp_cross_section().selection_stale());
  EXPECT_TRUE(view().shp_cross_section().preview_inputs_stale());

  ASSERT_TRUE(view().shp_cross_section().preview_selected().is_ok());
  EXPECT_TRUE(view().shp_cross_section().has_preview());
  EXPECT_FALSE(view().shp_cross_section().selection_stale());

  view().shp_cross_section().set_plane(Cross_section_plane::XZ);
  EXPECT_TRUE(view().shp_cross_section().preview_inputs_stale());
  ASSERT_TRUE(view().shp_cross_section().preview_selected().is_ok());
  EXPECT_FALSE(view().shp_cross_section().preview_inputs_stale());

  double offset_min = 0.0;
  double offset_max = 0.0;
  ASSERT_TRUE(view().shp_cross_section().try_get_offset_range_display(offset_min, offset_max));
  const double sample_offset = (offset_min + offset_max) * 0.25;
  view().shp_cross_section().set_offset_display(sample_offset);
  ASSERT_TRUE(view().shp_cross_section().preview_selected().is_ok());
  view().shp_cross_section().set_invert_normal(true);
  EXPECT_TRUE(view().shp_cross_section().get_invert_normal());
  EXPECT_NEAR(view().shp_cross_section().get_offset_display(), -sample_offset, 1e-9);
  EXPECT_TRUE(view().shp_cross_section().preview_inputs_stale());
  ASSERT_TRUE(view().shp_cross_section().preview_selected().is_ok());
  EXPECT_FALSE(view().shp_cross_section().preview_inputs_stale());

  view().shp_cross_section().set_hide_back_side(true);
  EXPECT_TRUE(view().shp_cross_section().get_hide_back_side());
  EXPECT_TRUE(view().shp_cross_section().preview_inputs_stale());
  ASSERT_TRUE(view().shp_cross_section().preview_selected().is_ok());
  EXPECT_FALSE(view().shp_cross_section().preview_inputs_stale());
  EXPECT_FALSE(boxes[1]->ClipPlanes().IsNull());
  EXPECT_EQ(boxes[1]->ClipPlanes()->Size(), 1);

  const Shape_id clipped_id = boxes[1]->get_id();
  ASSERT_TRUE(view().shp_cross_section().clip_selected().is_ok());
  EXPECT_FALSE(view().shp_cross_section().has_preview());
  EXPECT_TRUE(view().find_shape_by_id(clipped_id).IsNull());
  ASSERT_FALSE(view().get_shapes().empty());
  EXPECT_TRUE(contains_solid_like(view().get_shapes().back()->Shape()));
}


// ---------------------------------------------------------------------------
// shp_info
// ---------------------------------------------------------------------------

TEST(Shp_info, Collect_null_shape)
{
  const auto lines = shp_info::collect(TopoDS_Shape());
  EXPECT_EQ(line_value(lines, "Shape"), "null");
}

TEST(Shp_info, Collect_box_with_display_meta)
{
  const TopoDS_Shape     box = shp_create::create_box(0, 0, 0, 2, 3, 4);
  shp_info::Display_meta meta{"Box1", "Steel", "Shaded", true};
  const auto             lines = shp_info::collect(box, &meta);

  EXPECT_EQ(line_value(lines, "Name"), "Box1");
  EXPECT_EQ(line_value(lines, "Material"), "Steel");
  EXPECT_EQ(line_value(lines, "Display"), "Shaded");
  EXPECT_EQ(line_value(lines, "Visible"), "yes");
  EXPECT_EQ(line_value(lines, "Valid"), "yes");
  EXPECT_FALSE(line_value(lines, "Root type").empty());
  EXPECT_EQ(line_value(lines, "Solids"), "1");
  EXPECT_EQ(line_value(lines, "Volume"), "24");
}

// ---------------------------------------------------------------------------
// Occt_view registration and Shp metadata
// ---------------------------------------------------------------------------

TEST_F(Shp_test, AddBox_registers_named_solid)
{
  EXPECT_TRUE(view().get_shapes().empty());

  view().add_box(0, 0, 0, 10, 20, 30);
  ASSERT_EQ(view().get_shapes().size(), 1u);

  const Shp_ptr& shp = view().get_shapes().back();
  EXPECT_EQ(shp->get_name(), "Box");
  EXPECT_TRUE(shp->get_visible());
  EXPECT_EQ(shp->get_disp_mode(), AIS_Shaded);
  EXPECT_EQ(shp->Shape().ShapeType(), TopAbs_SOLID);
  EXPECT_NEAR(volume_of(shp->Shape()), 10.0 * 20.0 * 30.0, 1e-6);
  EXPECT_TRUE(shp->get_frame().Location().IsEqual(gp_Pnt(5, 10, 15), 1e-6));
}

TEST_F(Shp_test, UniqueShapeNames_increment)
{
  EXPECT_EQ(view().get_unique_shape_name("Box"), "Box");

  view().add_box(0, 0, 0, 1, 1, 1);
  EXPECT_EQ(view().get_unique_shape_name("Box"), "Box.001");

  view().add_box(2, 0, 0, 1, 1, 1);
  EXPECT_EQ(view().get_unique_shape_name("Box"), "Box.002");
  EXPECT_EQ(view().get_shapes().front()->get_name(), "Box");
  EXPECT_EQ(view().get_shapes().back()->get_name(), "Box.001");
}

TEST_F(Shp_test, AddSphere_and_SetVisible)
{
  view().add_sphere(0, 0, 0, 2.0);
  ASSERT_EQ(view().get_shapes().size(), 1u);

  Shp_ptr shp = view().get_shapes().back();
  EXPECT_EQ(shp->get_name(), "Sphere");
  EXPECT_NEAR(volume_of(shp->Shape()), (4.0 / 3.0) * std::numbers::pi * 8.0, 1e-3);

  shp->set_visible(false);
  EXPECT_FALSE(shp->get_visible());
  shp->set_visible(true);
  EXPECT_TRUE(shp->get_visible());
}

TEST_F(Shp_test, AddCylinder_Cone_Torus_Pyramid)
{
  view().add_cylinder(0, 0, 0, 1.0, 2.0);
  view().add_cone(5, 0, 0, 2.0, 1.0, 3.0);
  view().add_torus(10, 0, 0, 3.0, 0.5);
  view().add_pyramid(15, 0, 0, 2.0);

  ASSERT_EQ(view().get_shapes().size(), 4u);
  auto it = view().get_shapes().begin();
  EXPECT_EQ((*it++)->get_name(), "Cylinder");
  EXPECT_EQ((*it++)->get_name(), "Cone");
  EXPECT_EQ((*it++)->get_name(), "Torus");
  EXPECT_EQ((*it)->get_name(), "Pyramid");

  for (const Shp_ptr& shp : view().get_shapes())
  {
    EXPECT_FALSE(shp->Shape().IsNull());
    EXPECT_EQ(shp->Shape().ShapeType(), TopAbs_SOLID);
    EXPECT_GT(volume_of(shp->Shape()), 0.0);
  }
}

// ---------------------------------------------------------------------------
// Boolean operations (AIS selection injection)
// ---------------------------------------------------------------------------

TEST_F(Shp_test, Fuse_two_overlapping_boxes)
{
  view().add_box(0, 0, 0, 10, 10, 10);
  view().add_box(5, 0, 0, 10, 10, 10);
  ASSERT_EQ(view().get_shapes().size(), 2u);

  std::vector<Shp_ptr> to_select(view().get_shapes().begin(), view().get_shapes().end());
  select_shapes(view(), to_select);

  Status st = view().shp_fuse().selected_fuse();
  ASSERT_TRUE(st.is_ok()) << st.message();

  ASSERT_EQ(view().get_shapes().size(), 1u);
  const Shp_ptr& fused = view().get_shapes().back();
  EXPECT_EQ(fused->get_name(), "Fused");
  // Overlap is a 5x10x10 prism; union volume = 1000 + 1000 - 500 = 1500
  EXPECT_NEAR(volume_of(fused->Shape()), 1500.0, 1e-3);
}

TEST_F(Shp_test, Cut_box_from_box)
{
  view().add_box(0, 0, 0, 10, 10, 10);
  view().add_box(0, 0, 0, 5, 10, 10);
  ASSERT_EQ(view().get_shapes().size(), 2u);

  std::vector<Shp_ptr> to_select(view().get_shapes().begin(), view().get_shapes().end());
  select_shapes(view(), to_select);

  Status st = view().shp_cut().selected_cut();
  ASSERT_TRUE(st.is_ok()) << st.message();

  ASSERT_EQ(view().get_shapes().size(), 1u);
  EXPECT_EQ(view().get_shapes().back()->get_name(), "Cut");
  EXPECT_NEAR(volume_of(view().get_shapes().back()->Shape()), 500.0, 1e-3);
}

TEST_F(Shp_test, Common_overlapping_boxes)
{
  view().add_box(0, 0, 0, 10, 10, 10);
  view().add_box(5, 0, 0, 10, 10, 10);
  ASSERT_EQ(view().get_shapes().size(), 2u);

  std::vector<Shp_ptr> to_select(view().get_shapes().begin(), view().get_shapes().end());
  select_shapes(view(), to_select);

  Status st = view().shp_common().selected_common();
  ASSERT_TRUE(st.is_ok()) << st.message();

  ASSERT_EQ(view().get_shapes().size(), 1u);
  EXPECT_EQ(view().get_shapes().back()->get_name(), "Common");
  EXPECT_NEAR(volume_of(view().get_shapes().back()->Shape()), 500.0, 1e-3);
}

TEST_F(Shp_test, Fuse_requires_two_shapes)
{
  view().add_box(0, 0, 0, 1, 1, 1);
  select_shapes(view(), {view().get_shapes().back()});

  Status st = view().shp_fuse().selected_fuse();
  EXPECT_FALSE(st.is_ok());
  EXPECT_EQ(view().get_shapes().size(), 1u);
}

// ---------------------------------------------------------------------------
// Shape undo / redo (typed deltas, no full-document snapshot)
// ---------------------------------------------------------------------------

TEST_F(Shp_test, Undo_add_box_removes_shape)
{
  EXPECT_EQ(view().get_shapes().size(), 0u);
  view().add_box(0, 0, 0, 10, 10, 10);
  ASSERT_EQ(view().get_shapes().size(), 1u);
  const Shape_id id = view().get_shapes().back()->get_id();
  EXPECT_NE(id, 0u);

  EXPECT_TRUE(view().undo());
  EXPECT_EQ(view().get_shapes().size(), 0u);

  EXPECT_TRUE(view().redo());
  ASSERT_EQ(view().get_shapes().size(), 1u);
  EXPECT_EQ(view().get_shapes().back()->get_id(), id);
  EXPECT_NEAR(volume_of(view().get_shapes().back()->Shape()), 1000.0, 1e-6);
}

TEST_F(Shp_test, Undo_delete_shape_restores_brep)
{
  view().add_box(0, 0, 0, 10, 10, 10);
  ASSERT_EQ(view().get_shapes().size(), 1u);
  const Shape_id id = view().get_shapes().back()->get_id();

  std::vector<AIS_Shape_ptr> to_delete;
  to_delete.push_back(view().get_shapes().back());
  view().delete_shapes(to_delete);
  EXPECT_EQ(view().get_shapes().size(), 0u);

  EXPECT_TRUE(view().undo());
  ASSERT_EQ(view().get_shapes().size(), 1u);
  EXPECT_EQ(view().get_shapes().back()->get_id(), id);
  EXPECT_NEAR(volume_of(view().get_shapes().back()->Shape()), 1000.0, 1e-6);
}

TEST_F(Shp_test, Undo_fuse_restores_inputs)
{
  view().add_box(0, 0, 0, 10, 10, 10);
  view().add_box(5, 0, 0, 10, 10, 10);
  ASSERT_EQ(view().get_shapes().size(), 2u);
  const Shape_id id_a = view().get_shapes().front()->get_id();
  const Shape_id id_b = view().get_shapes().back()->get_id();

  std::vector<Shp_ptr> to_select(view().get_shapes().begin(), view().get_shapes().end());
  select_shapes(view(), to_select);
  ASSERT_TRUE(view().shp_fuse().selected_fuse().is_ok());
  ASSERT_EQ(view().get_shapes().size(), 1u);

  EXPECT_TRUE(view().undo());
  ASSERT_EQ(view().get_shapes().size(), 2u);
  EXPECT_FALSE(view().find_shape_by_id(id_a).IsNull());
  EXPECT_FALSE(view().find_shape_by_id(id_b).IsNull());

  EXPECT_TRUE(view().redo());
  ASSERT_EQ(view().get_shapes().size(), 1u);
  EXPECT_NEAR(volume_of(view().get_shapes().back()->Shape()), 1500.0, 1e-3);
}

TEST_F(Shp_test, Undo_interleaves_sketch_delta_and_shape_add)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  {
    Sketch_op_recorder rec(view(), sketch);
    Sketch_access::add_edge_(sketch, gp_Pnt2d(0.0, 0.0), gp_Pnt2d(10.0, 0.0), rec);
    rec.commit();
  }

  view().add_box(0, 0, 0, 2, 2, 2);
  EXPECT_EQ(view().get_shapes().size(), 1u);
  EXPECT_EQ(Sketch_access::get_linear_edge_count(sketch), 1u);

  EXPECT_TRUE(view().undo()); // undo box
  EXPECT_EQ(view().get_shapes().size(), 0u);
  EXPECT_EQ(Sketch_access::get_linear_edge_count(sketch), 1u);

  EXPECT_TRUE(view().undo()); // undo edge
  EXPECT_EQ(Sketch_access::get_linear_edge_count(sketch), 0u);

  EXPECT_TRUE(view().redo());
  EXPECT_EQ(Sketch_access::get_linear_edge_count(sketch), 1u);
  EXPECT_TRUE(view().redo());
  EXPECT_EQ(view().get_shapes().size(), 1u);
}

TEST_F(Shp_test, Shape_ids_persist_in_json)
{
  view().add_box(0, 0, 0, 3, 4, 5);
  ASSERT_EQ(view().get_shapes().size(), 1u);
  const Shape_id id = view().get_shapes().back()->get_id();

  const std::string json = view().to_json();
  view().new_file();
  EXPECT_EQ(view().get_shapes().size(), 0u);

  view().load(json, false);
  ASSERT_EQ(view().get_shapes().size(), 1u);
  EXPECT_EQ(view().get_shapes().back()->get_id(), id);
}

TEST_F(Shp_test, Shape_frame_persists_in_json_and_undo)
{
  view().add_box(10, 20, 30, 4, 6, 8);
  ASSERT_EQ(view().get_shapes().size(), 1u);
  const gp_Ax3 expected = view().get_shapes().back()->get_frame();

  EXPECT_TRUE(view().undo());
  EXPECT_TRUE(view().get_shapes().empty());
  EXPECT_TRUE(view().redo());
  ASSERT_EQ(view().get_shapes().size(), 1u);
  EXPECT_TRUE(view().get_shapes().back()->get_frame().Location().IsEqual(expected.Location(), 1e-6));

  const std::string json = view().to_json();
  view().new_file();
  view().load(json, false);
  ASSERT_EQ(view().get_shapes().size(), 1u);
  EXPECT_TRUE(view().get_shapes().back()->get_frame().Location().IsEqual(expected.Location(), 1e-6));
  EXPECT_TRUE(view().get_shapes().back()->get_frame().Direction().IsEqual(expected.Direction(), 1e-9));
  EXPECT_TRUE(view().get_shapes().back()->get_frame().XDirection().IsEqual(expected.XDirection(), 1e-9));
}

// ---------------------------------------------------------------------------
// Shape hierarchy (organizational groups)
// ---------------------------------------------------------------------------

TEST_F(Shp_test, Group_reparent_cycle_rejected)
{
  view().add_box(0, 0, 0, 1, 1, 1);
  view().add_box(2, 0, 0, 1, 1, 1);
  Shp_ptr a = view().get_shapes().front();
  Shp_ptr b = view().get_shapes().back();

  ASSERT_TRUE(view().group_shapes({a, b}).is_ok());
  Shp_ptr grp;
  for (const Shp_ptr& s : view().get_shapes())
    if (s->is_group())
      grp = s;

  ASSERT_FALSE(grp.IsNull());
  EXPECT_TRUE(view().would_reparent_create_cycle(grp->get_id(), a->get_id()));
  EXPECT_FALSE(view().reparent_shape(grp->get_id(), a->get_id()).is_ok());
}

TEST_F(Shp_test, Ungroup_moves_all_direct_children)
{
  view().add_box(0, 0, 0, 1, 1, 1);
  view().add_box(2, 0, 0, 1, 1, 1);
  view().add_box(4, 0, 0, 1, 1, 1);
  ASSERT_EQ(view().get_shapes().size(), 3u);

  Shp_ptr grp = view().create_group("Group", 0);
  ASSERT_FALSE(grp.IsNull());
  const Shape_id gid = grp->get_id();

  std::vector<Shp_ptr> boxes;
  for (const Shp_ptr& s : view().get_shapes())
    if (!s->is_group())
      boxes.push_back(s);

  ASSERT_EQ(boxes.size(), 3u);
  for (const Shp_ptr& b : boxes)
    ASSERT_TRUE(view().reparent_shape(b->get_id(), gid).is_ok());

  EXPECT_EQ(view().shape_children(gid).size(), 3u);

  ASSERT_TRUE(view().ungroup_shape(gid).is_ok());
  EXPECT_TRUE(view().find_shape_by_id(gid).IsNull());
  EXPECT_EQ(view().shape_children(0).size(), 3u);
  for (const Shp_ptr& s : view().get_shapes())
  {
    EXPECT_FALSE(s->is_group());
    EXPECT_EQ(s->get_parent_id(), 0u);
  }
}

TEST_F(Shp_test, Group_ungroup_and_cascade_delete_undo)
{
  view().add_box(0, 0, 0, 1, 1, 1);
  view().add_box(2, 0, 0, 1, 1, 1);
  std::vector<Shp_ptr> boxes(view().get_shapes().begin(), view().get_shapes().end());
  ASSERT_TRUE(view().group_shapes(boxes).is_ok());

  Shp_ptr grp;
  for (const Shp_ptr& s : view().get_shapes())
    if (s->is_group())
      grp = s;

  ASSERT_FALSE(grp.IsNull());
  EXPECT_EQ(view().shape_children(grp->get_id()).size(), 2u);

  ASSERT_TRUE(view().ungroup_shape(grp->get_id()).is_ok());
  EXPECT_EQ(view().shape_children(0).size(), 2u);
  for (const Shp_ptr& s : view().get_shapes())
    EXPECT_FALSE(s->is_group());

  EXPECT_TRUE(view().undo()); // undo ungroup -> group back
  size_t groups = 0;
  for (const Shp_ptr& s : view().get_shapes())
    if (s->is_group())
      ++groups;

  EXPECT_EQ(groups, 1u);

  for (const Shp_ptr& s : view().get_shapes())
    if (s->is_group())
      grp = s;

  view().delete_shapes({grp});
  EXPECT_TRUE(view().get_shapes().empty());

  EXPECT_TRUE(view().undo());
  EXPECT_EQ(view().get_shapes().size(), 3u); // group + 2 boxes
}

TEST_F(Shp_test, Hierarchy_json_round_trip)
{
  view().add_box(0, 0, 0, 1, 1, 1);
  view().add_box(2, 0, 0, 1, 1, 1);
  std::vector<Shp_ptr> boxes(view().get_shapes().begin(), view().get_shapes().end());
  ASSERT_TRUE(view().group_shapes(boxes).is_ok());

  const std::string json = view().to_json();
  view().new_file();
  view().load(json, false);

  size_t   groups   = 0;
  size_t   leaves   = 0;
  Shape_id group_id = 0;
  for (const Shp_ptr& s : view().get_shapes())
  {
    if (s->is_group())
    {
      ++groups;
      group_id = s->get_id();
    }
    else
      ++leaves;
  }
  EXPECT_EQ(groups, 1u);
  EXPECT_EQ(leaves, 2u);
  EXPECT_EQ(view().shape_children(group_id).size(), 2u);
  for (const Shp_ptr& c : view().shape_children(group_id))
    EXPECT_EQ(c->get_parent_id(), group_id);
}

TEST_F(Shp_test, Hide_all_preserves_per_shape_visibility)
{
  view().add_box(0, 0, 0, 1, 1, 1);
  Shp_ptr shp = view().get_shapes().back();
  shp->set_visible(false);
  EXPECT_FALSE(shp->get_visible());

  gui().set_hide_all_shapes(true);
  view().sync_sketch_shape_faint_style();
  EXPECT_FALSE(shp->get_visible()); // preference unchanged

  gui().set_hide_all_shapes(false);
  view().sync_sketch_shape_faint_style();
  EXPECT_FALSE(shp->get_visible());
}

TEST_F(Shp_test, Fuse_keeps_shared_parent)
{
  view().add_box(0, 0, 0, 10, 10, 10);
  view().add_box(5, 0, 0, 10, 10, 10);
  std::vector<Shp_ptr> boxes(view().get_shapes().begin(), view().get_shapes().end());
  ASSERT_TRUE(view().group_shapes(boxes).is_ok());

  Shp_ptr grp;
  for (const Shp_ptr& s : view().get_shapes())
    if (s->is_group())
      grp = s;

  ASSERT_FALSE(grp.IsNull());
  std::vector<Shp_ptr> kids = view().shape_children(grp->get_id());
  ASSERT_EQ(kids.size(), 2u);
  select_shapes(view(), kids);
  ASSERT_TRUE(view().shp_fuse().selected_fuse().is_ok());

  Shp_ptr fused;
  for (const Shp_ptr& s : view().get_shapes())
    if (!s->is_group())
      fused = s;

  ASSERT_FALSE(fused.IsNull());
  EXPECT_EQ(fused->get_parent_id(), grp->get_id());
}

TEST_F(Shp_test, Current_group_parents_new_primitives)
{
  Shp_ptr grp = view().create_group("Group", 0);
  ASSERT_FALSE(grp.IsNull());
  view().set_current_group_id(grp->get_id());
  EXPECT_EQ(view().current_group_id(), grp->get_id());

  view().add_box(0, 0, 0, 1, 1, 1);
  Shp_ptr box;
  for (const Shp_ptr& s : view().get_shapes())
    if (!s->is_group())
      box = s;

  ASSERT_FALSE(box.IsNull());
  EXPECT_EQ(box->get_parent_id(), grp->get_id());

  view().set_current_group_id(0);
  view().add_box(2, 0, 0, 1, 1, 1);
  Shp_ptr root_box;
  for (const Shp_ptr& s : view().get_shapes())
    if (!s->is_group() && s->get_parent_id() == 0)
      root_box = s;

  ASSERT_FALSE(root_box.IsNull());
  EXPECT_EQ(root_box->get_parent_id(), 0u);

  view().set_current_group_id(grp->get_id());
  ASSERT_TRUE(view().ungroup_shape(grp->get_id()).is_ok());
  EXPECT_EQ(view().current_group_id(), 0u);
}
