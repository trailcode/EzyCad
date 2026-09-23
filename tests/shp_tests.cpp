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
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <NCollection_List.hxx>
#include <V3d_View.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax3.hxx>
#include <gp_Lin.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Trsf.hxx>
#include <cmath>
#include <optional>
#include <numbers>

#include "shp.h"
#include "shp_create.h"
#include "shp_info.h"
#include "shp_cross_section.h"
#include "shp_rotate.h"
#include "shp_scale.h"
#include "shp_transform.h"
#include "utl_geom.h"
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

int displayed_object_count(AIS_InteractiveContext& ctx)
{
  NCollection_List<AIS_InteractiveObject_ptr> displayed;
  ctx.DisplayedObjects(displayed);
  return displayed.Extent();
}
} // namespace

class Shp_rotate_access
{
public:
  static void capture_drag_frame(Shp_rotate& rotate)
  {
    rotate.capture_drag_frame_();
  }

  static Status ensure_start(Shp_rotate& rotate)
  {
    return rotate.ensure_start_state_();
  }

  static Status apply_world(Shp_rotate& rotate, const gp_Pnt& mouse_wc_pos, const gp_Dir& axis_dir, const gp_Pln& pln)
  {
    return rotate.update_rotate_from_world_(mouse_wc_pos, axis_dir, pln);
  }

  static double angle(const Shp_rotate& rotate)
  {
    return rotate.m_angle;
  }

  static const std::optional<gp_Pnt>& initial_mouse_pos(const Shp_rotate& rotate)
  {
    return rotate.m_initial_mouse_pos;
  }

  static const std::optional<gp_Pnt>& center(const Shp_rotate& rotate)
  {
    return rotate.m_center;
  }

  static const std::optional<gp_Pln>& rotate_pln(const Shp_rotate& rotate)
  {
    return rotate.m_rotate_pln;
  }

  static const std::optional<gp_Dir>& captured_axis_dir(const Shp_rotate& rotate)
  {
    return rotate.m_captured_axis_dir;
  }
};

class Shp_scale_access
{
public:
  static Status ensure_start(Shp_scale& scale)
  {
    return scale.ensure_start_state_();
  }

  static Status apply_distance(Shp_scale& scale, double dist)
  {
    return scale.update_scale_from_distance_(dist);
  }

  static double scale_factor(const Shp_scale& scale)
  {
    return scale.m_scale_factor;
  }

  static double initial_distance(const Shp_scale& scale)
  {
    return scale.m_initial_distance;
  }

  static const std::optional<gp_Pnt>& center(const Shp_scale& scale)
  {
    return scale.m_center;
  }
};

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

  // Hide back side defaults on (already acked by prior preview); toggle to exercise stale + clip apply.
  EXPECT_TRUE(view().shp_cross_section().get_hide_back_side());
  EXPECT_FALSE(boxes[1]->ClipPlanes().IsNull());
  EXPECT_EQ(boxes[1]->ClipPlanes()->Size(), 1);

  view().shp_cross_section().set_hide_back_side(false);
  EXPECT_TRUE(view().shp_cross_section().preview_inputs_stale());
  ASSERT_TRUE(view().shp_cross_section().preview_selected().is_ok());
  EXPECT_FALSE(view().shp_cross_section().preview_inputs_stale());
  EXPECT_TRUE(boxes[1]->ClipPlanes().IsNull() || boxes[1]->ClipPlanes()->IsEmpty());

  view().shp_cross_section().set_hide_back_side(true);
  EXPECT_TRUE(view().shp_cross_section().preview_inputs_stale());
  ASSERT_TRUE(view().shp_cross_section().preview_selected().is_ok());
  EXPECT_FALSE(view().shp_cross_section().preview_inputs_stale());
  EXPECT_FALSE(boxes[1]->ClipPlanes().IsNull());
  EXPECT_EQ(boxes[1]->ClipPlanes()->Size(), 1);

  // Outline defaults off; toggle shows/hides cyan AIS without a full recompute (not a preview-input).
  EXPECT_FALSE(view().shp_cross_section().get_show_section_outline());
  EXPECT_TRUE(view().shp_cross_section().has_preview());
  view().shp_cross_section().set_show_section_outline(true);
  EXPECT_TRUE(view().shp_cross_section().has_preview());
  EXPECT_FALSE(view().shp_cross_section().preview_inputs_stale());
  view().shp_cross_section().set_show_section_outline(false);
  EXPECT_TRUE(view().shp_cross_section().has_preview());
  EXPECT_FALSE(view().shp_cross_section().preview_inputs_stale());

  const Shape_id clipped_id = boxes[1]->get_id();
  ASSERT_TRUE(view().shp_cross_section().clip_selected().is_ok());
  EXPECT_FALSE(view().shp_cross_section().has_preview());
  // Clip reuses the solid's Shape_id so workbench links stay attached.
  const Shp_ptr clipped = view().find_shape_by_id(clipped_id);
  EXPECT_FALSE(clipped.IsNull());
  ASSERT_FALSE(view().get_shapes().empty());
  EXPECT_TRUE(contains_solid_like(clipped->Shape()));
  EXPECT_FALSE(view().find_shape_by_id(boxes[0]->get_id()).IsNull());
}

TEST_F(Shp_test, Cross_section_clip_removes_fully_discarded_solids)
{
  // Short box lies entirely below the shared midplane; tall box is cut (kept half survives).
  view().add_box(0, 0, 0, 4, 4, 4);
  view().add_box(0, 0, 0, 4, 4, 10);
  const std::vector<Shp_ptr> boxes(view().get_shapes().begin(), view().get_shapes().end());
  ASSERT_EQ(boxes.size(), 2u);
  const Shape_id short_id = boxes[0]->get_id();
  const Shape_id tall_id  = boxes[1]->get_id();

  select_shapes(view(), boxes);
  gui().set_mode(Mode::Shape_cross_section);
  ASSERT_TRUE(view().shp_cross_section().preview_selected().is_ok());

  const Status clip_status = view().shp_cross_section().clip_selected();
  ASSERT_TRUE(clip_status.is_ok()) << clip_status.message();
  EXPECT_TRUE(view().find_shape_by_id(short_id).IsNull());
  // The cut solid keeps its id; only the fully discarded box is removed.
  EXPECT_FALSE(view().find_shape_by_id(tall_id).IsNull());
  ASSERT_EQ(view().get_shapes().size(), 1u);
  EXPECT_EQ(view().get_shapes().front()->get_id(), tall_id);
  EXPECT_TRUE(contains_solid_like(view().get_shapes().front()->Shape()));
}

TEST_F(Shp_test, Cross_section_sketch_imports_box_midplane_lines)
{
  view().add_box(0, 0, 0, 10, 10, 10);
  select_shapes(view(), {view().get_shapes().back()});
  gui().set_mode(Mode::Shape_cross_section);
  ASSERT_TRUE(view().shp_cross_section().preview_selected().is_ok());
  ASSERT_TRUE(view().shp_cross_section().has_preview());

  const size_t sketches_before = view().get_sketches().size();
  const Status status          = view().create_sketch_from_cross_section();
  ASSERT_TRUE(status.is_ok()) << status.message();
  EXPECT_EQ(view().get_sketches().size(), sketches_before + 1u);
  EXPECT_EQ(gui().get_mode(), Mode::Sketch_inspection);
  EXPECT_GE(Sketch_access::get_linear_edge_count(view().curr_sketch()), 4u);
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

TEST_F(Shp_test, Cut_keeps_object_id_and_workbench_links)
{
  view().add_box(0, 0, 0, 10, 10, 10);
  view().add_box(0, 0, 0, 5, 10, 10);
  Shp_ptr        object    = *view().get_shapes().begin();
  Shp_ptr        tool      = view().get_shapes().back();
  const Shape_id object_id = object->get_id();
  ASSERT_TRUE(view().add_to_workbench({object}).is_ok());
  ASSERT_TRUE(view().add_to_workbench({object}).is_ok());
  ASSERT_TRUE(view().add_to_workbench({tool}).is_ok());
  ASSERT_EQ(view().get_workbench_shapes().size(), 3u);

  std::vector<Shp_ptr> to_select(view().get_shapes().begin(), view().get_shapes().end());
  select_shapes(view(), to_select);
  Status st = view().shp_cut().selected_cut();
  ASSERT_TRUE(st.is_ok()) << st.message();

  ASSERT_EQ(view().get_shapes().size(), 1u);
  EXPECT_EQ(view().get_shapes().back()->get_id(), object_id);
  ASSERT_EQ(view().get_workbench_shapes().size(), 2u);
  for (const Shp_ptr& inst : view().get_workbench_shapes())
  {
    EXPECT_EQ(inst->get_source_id(), object_id);
    EXPECT_NEAR(volume_of(inst->Shape()), 500.0, 1e-3);
  }

  EXPECT_TRUE(view().undo());
  ASSERT_EQ(view().get_shapes().size(), 2u);
  Shp_ptr restored;
  for (const Shp_ptr& s : view().get_shapes())
    if (s->get_id() == object_id)
      restored = s;

  ASSERT_FALSE(restored.IsNull());
  EXPECT_NEAR(volume_of(restored->Shape()), 1000.0, 1e-3);
  ASSERT_EQ(view().get_workbench_shapes().size(), 2u);
  for (const Shp_ptr& inst : view().get_workbench_shapes())
  {
    EXPECT_EQ(inst->get_source_id(), object_id);
    EXPECT_NEAR(volume_of(inst->Shape()), 1000.0, 1e-3);
  }

  EXPECT_TRUE(view().redo());
  ASSERT_EQ(view().get_shapes().size(), 1u);
  EXPECT_EQ(view().get_shapes().back()->get_id(), object_id);
  ASSERT_EQ(view().get_workbench_shapes().size(), 2u);
  EXPECT_NEAR(volume_of(view().get_workbench_shapes().back()->Shape()), 500.0, 1e-3);
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

TEST_F(Shp_test, Delete_shape_clears_frame_ais)
{
  // Static GUI can be left in a sketch mode by an earlier test; that suppresses frame AIS.
  gui().set_mode(Mode::Design_inspection);

  view().add_box(0, 0, 0, 10, 10, 10);
  ASSERT_EQ(view().get_shapes().size(), 1u);
  const Shp_ptr shp       = view().get_shapes().back();
  const int     after_box = displayed_object_count(view().ctx());

  shp->set_show_frame_axes(true);
  shp->set_show_frame_plane(true);
  shp->set_show_frame_up(true);
  const int with_frame = displayed_object_count(view().ctx());
  ASSERT_GT(with_frame, after_box);

  view().delete_shapes({shp});
  EXPECT_EQ(view().get_shapes().size(), 0u);
  EXPECT_EQ(displayed_object_count(view().ctx()), after_box - 1);

  EXPECT_TRUE(view().undo());
  ASSERT_EQ(view().get_shapes().size(), 1u);
  EXPECT_TRUE(view().get_shapes().back()->show_frame_axes());
  EXPECT_TRUE(view().get_shapes().back()->show_frame_plane());
  EXPECT_TRUE(view().get_shapes().back()->show_frame_up());
  EXPECT_EQ(displayed_object_count(view().ctx()), with_frame);
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

TEST_F(Shp_test, Set_frame_undo_stays_in_design_inspection)
{
  gui().set_mode(Mode::Design_inspection);
  gui().set_hide_all_shapes(false);

  view().add_box(0, 0, 0, 10, 10, 10);
  Shp_ptr shp = view().get_shapes().back();
  ASSERT_FALSE(shp.IsNull());
  const gp_Ax3 before = shp->get_frame();
  gp_Ax3       after  = before;
  after.ZReverse();

  gui().set_mode(Mode::Shape_set_frame);
  view().shp_set_frame().begin(shp, Shp_set_frame::Pick::Planar_face);
  ASSERT_TRUE(view().shp_set_frame().has_target());

  // Same order as the old pick() bug: push undo while still in Shape_set_frame, then leave.
  view().set_shape_frame(shp, after);
  shp->set_show_frame_axes(true);
  view().shp_set_frame().cancel();
  EXPECT_EQ(gui().get_mode(), Mode::Design_inspection);
  EXPECT_FALSE(view().shp_set_frame().has_target());

  EXPECT_TRUE(view().undo());
  EXPECT_EQ(gui().get_mode(), Mode::Design_inspection);
  EXPECT_FALSE(view().shp_set_frame().has_target());
  EXPECT_TRUE(shp->get_frame().Direction().IsEqual(before.Direction(), 1e-9));

  EXPECT_TRUE(view().redo());
  EXPECT_EQ(gui().get_mode(), Mode::Design_inspection);
  EXPECT_TRUE(shp->get_frame().Direction().IsEqual(after.Direction(), 1e-9));
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

TEST_F(Shp_test, Parent_chain_walk_tolerates_corrupt_cycle)
{
  // Defensive: a corrupt parent loop must not hang ancestor walks used by reparent
  // and copy/paste collapse / paste-into-subtree checks.
  Shp_ptr g1 = view().create_group("G1", 0);
  Shp_ptr g2 = view().create_group("G2", 0);
  ASSERT_FALSE(g1.IsNull());
  ASSERT_FALSE(g2.IsNull());
  g1->set_parent_id(g2->get_id());
  g2->set_parent_id(g1->get_id());

  EXPECT_TRUE(view().would_reparent_create_cycle(g1->get_id(), g2->get_id()));
  EXPECT_FALSE(view().would_reparent_create_cycle(g1->get_id(), 0));

  view().add_box(0, 0, 0, 1, 1, 1);
  Shp_ptr box;
  for (const Shp_ptr& s : view().get_shapes())
    if (!s->is_group())
      box = s;
  ASSERT_FALSE(box.IsNull());
  select_shapes(view(), {box});
  ASSERT_TRUE(view().copy_selected_shapes().is_ok());
  view().set_current_group_id(g1->get_id());
  // Paste under a node trapped in a parent cycle must terminate (ok or error).
  (void)view().paste_clipboard_shapes();
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

TEST_F(Shp_test, Hide_all_clears_frame_ais)
{
  gui().set_mode(Mode::Design_inspection);
  gui().set_hide_all_shapes(false);

  view().add_box(0, 0, 0, 10, 10, 10);
  ASSERT_EQ(view().get_shapes().size(), 1u);
  const Shp_ptr shp       = view().get_shapes().back();
  const int     after_box = displayed_object_count(view().ctx());

  shp->set_show_frame_axes(true);
  shp->set_show_frame_plane(true);
  shp->set_show_frame_up(true);
  const int with_frame = displayed_object_count(view().ctx());
  ASSERT_GT(with_frame, after_box);

  gui().set_hide_all_shapes(true);
  view().sync_sketch_shape_faint_style();
  EXPECT_TRUE(shp->get_visible());
  EXPECT_TRUE(shp->show_frame_axes());
  EXPECT_EQ(displayed_object_count(view().ctx()), after_box - 1);

  shp->set_show_frame_axes(false);
  shp->set_show_frame_axes(true);
  EXPECT_EQ(displayed_object_count(view().ctx()), after_box - 1);

  gui().set_hide_all_shapes(false);
  view().sync_sketch_shape_faint_style();
  EXPECT_EQ(displayed_object_count(view().ctx()), with_frame);
}

TEST_F(Shp_test, Hidden_group_clears_child_frame_ais)
{
  gui().set_mode(Mode::Design_inspection);
  gui().set_hide_all_shapes(false);

  view().add_box(0, 0, 0, 10, 10, 10);
  Shp_ptr box = view().get_shapes().back();
  box->set_show_frame_axes(true);
  box->set_show_frame_plane(true);
  box->set_show_frame_up(true);
  const int with_frame = displayed_object_count(view().ctx());

  ASSERT_TRUE(view().group_shapes({box}).is_ok());
  Shp_ptr grp;
  for (const Shp_ptr& s : view().get_shapes())
    if (s->is_group())
      grp = s;

  ASSERT_FALSE(grp.IsNull());
  EXPECT_EQ(displayed_object_count(view().ctx()), with_frame);

  grp->set_visible(false);
  view().sync_sketch_shape_faint_style();
  EXPECT_TRUE(box->get_visible());
  EXPECT_TRUE(box->show_frame_axes());
  EXPECT_LT(displayed_object_count(view().ctx()), with_frame);

  grp->set_visible(true);
  view().sync_sketch_shape_faint_style();
  EXPECT_EQ(displayed_object_count(view().ctx()), with_frame);
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

// ---------------------------------------------------------------------------
// In-app shape copy / paste
// ---------------------------------------------------------------------------

TEST_F(Shp_test, Copy_paste_loose_solids_new_ids_and_undo)
{
  view().add_box(0, 0, 0, 1, 1, 1);
  view().add_box(2, 0, 0, 1, 1, 1);
  std::vector<Shp_ptr> boxes;
  for (const Shp_ptr& s : view().get_shapes())
    if (!s->is_group())
      boxes.push_back(s);

  ASSERT_EQ(boxes.size(), 2u);
  const Shape_id id0 = boxes[0]->get_id();
  const Shape_id id1 = boxes[1]->get_id();
  select_shapes(view(), boxes);

  ASSERT_TRUE(view().copy_selected_shapes().is_ok());
  EXPECT_TRUE(view().has_shape_clipboard());
  ASSERT_TRUE(view().paste_clipboard_shapes().is_ok());

  size_t leaves = 0;
  for (const Shp_ptr& s : view().get_shapes())
    if (!s->is_group())
      ++leaves;

  EXPECT_EQ(leaves, 4u);

  const std::vector<Shp_ptr> selected = view().get_selected_shps();
  ASSERT_EQ(selected.size(), 2u);
  for (const Shp_ptr& s : selected)
  {
    EXPECT_NE(s->get_id(), id0);
    EXPECT_NE(s->get_id(), id1);
    EXPECT_EQ(s->get_parent_id(), 0u);
  }

  EXPECT_TRUE(view().undo());
  leaves = 0;
  for (const Shp_ptr& s : view().get_shapes())
    if (!s->is_group())
      ++leaves;

  EXPECT_EQ(leaves, 2u);
}

TEST_F(Shp_test, Copy_paste_group_subtree_preserves_nesting)
{
  view().add_box(0, 0, 0, 1, 1, 1);
  view().add_box(2, 0, 0, 1, 1, 1);
  std::vector<Shp_ptr> boxes;
  for (const Shp_ptr& s : view().get_shapes())
    if (!s->is_group())
      boxes.push_back(s);

  ASSERT_TRUE(view().group_shapes(boxes).is_ok());
  Shp_ptr grp;
  for (const Shp_ptr& s : view().get_shapes())
    if (s->is_group())
      grp = s;

  ASSERT_FALSE(grp.IsNull());
  const Shape_id gid = grp->get_id();

  // Nested empty group under the outer group.
  Shp_ptr nested = view().create_group("Nested", gid);
  ASSERT_FALSE(nested.IsNull());

  // Mimic Shape List group click: current group + all descendant solids selected.
  view().set_current_group_id(gid);
  select_shapes(view(), view().shape_descendant_solids(gid));

  ASSERT_TRUE(view().copy_selected_shapes().is_ok());
  view().set_current_group_id(0);
  ASSERT_TRUE(view().paste_clipboard_shapes().is_ok());

  size_t groups = 0;
  size_t leaves = 0;
  for (const Shp_ptr& s : view().get_shapes())
  {
    if (s->is_group())
      ++groups;
    else
      ++leaves;
  }
  // Original: outer + nested; pasted: outer + nested.
  EXPECT_EQ(groups, 4u);
  EXPECT_EQ(leaves, 4u);

  // Pasted root group should be current and have two direct children (2 boxes or nested+boxes).
  const Shape_id pasted_gid = view().current_group_id();
  EXPECT_NE(pasted_gid, 0u);
  EXPECT_NE(pasted_gid, gid);
  Shp_ptr pasted_grp = view().find_shape_by_id(pasted_gid);
  ASSERT_FALSE(pasted_grp.IsNull());
  EXPECT_TRUE(pasted_grp->is_group());
  EXPECT_EQ(pasted_grp->get_parent_id(), 0u);

  size_t nested_under_paste = 0;
  size_t solids_under_paste = 0;
  for (const Shp_ptr& c : view().shape_children(pasted_gid))
  {
    if (c->is_group())
      ++nested_under_paste;
    else
      ++solids_under_paste;
  }
  EXPECT_EQ(nested_under_paste, 1u);
  EXPECT_EQ(solids_under_paste, 2u);
  EXPECT_EQ(view().shape_descendant_solids(pasted_gid).size(), 2u);
}

TEST_F(Shp_test, Copy_partial_group_selection_copies_solids_not_group)
{
  view().add_box(0, 0, 0, 1, 1, 1);
  view().add_box(2, 0, 0, 1, 1, 1);
  std::vector<Shp_ptr> boxes;
  for (const Shp_ptr& s : view().get_shapes())
    if (!s->is_group())
      boxes.push_back(s);

  ASSERT_TRUE(view().group_shapes(boxes).is_ok());
  Shp_ptr grp;
  for (const Shp_ptr& s : view().get_shapes())
    if (s->is_group())
      grp = s;

  ASSERT_FALSE(grp.IsNull());
  view().set_current_group_id(grp->get_id());
  // Only one solid selected -> do not treat as whole-group copy.
  select_shapes(view(), {boxes[0]});

  ASSERT_TRUE(view().copy_selected_shapes().is_ok());
  view().set_current_group_id(0);
  ASSERT_TRUE(view().paste_clipboard_shapes().is_ok());

  size_t groups = 0;
  size_t leaves = 0;
  for (const Shp_ptr& s : view().get_shapes())
  {
    if (s->is_group())
      ++groups;
    else
      ++leaves;
  }
  EXPECT_EQ(groups, 1u);
  EXPECT_EQ(leaves, 3u);

  const std::vector<Shp_ptr> selected = view().get_selected_shps();
  ASSERT_EQ(selected.size(), 1u);
  EXPECT_EQ(selected[0]->get_parent_id(), 0u);
}

TEST_F(Shp_test, Copy_paste_group_while_still_current_group)
{
  // User workflow: click group in Shape List (current_group = that group), Ctrl+C, Ctrl+V
  // without changing the current group first. Paste must be a sibling, not a child of itself.
  view().add_box(0, 0, 0, 1, 1, 1);
  view().add_box(2, 0, 0, 1, 1, 1);
  std::vector<Shp_ptr> boxes;
  for (const Shp_ptr& s : view().get_shapes())
    if (!s->is_group())
      boxes.push_back(s);

  ASSERT_TRUE(view().group_shapes(boxes).is_ok());
  Shp_ptr grp;
  for (const Shp_ptr& s : view().get_shapes())
    if (s->is_group())
      grp = s;

  ASSERT_FALSE(grp.IsNull());
  const Shape_id gid = grp->get_id();
  view().set_current_group_id(gid);
  select_shapes(view(), view().shape_descendant_solids(gid));

  ASSERT_TRUE(view().copy_selected_shapes().is_ok());
  // Intentionally leave current_group_id == gid.
  ASSERT_EQ(view().current_group_id(), gid);
  ASSERT_TRUE(view().paste_clipboard_shapes().is_ok());

  size_t groups = 0;
  size_t leaves = 0;
  for (const Shp_ptr& s : view().get_shapes())
  {
    ASSERT_FALSE(s.IsNull());
    if (s->is_group())
      ++groups;
    else
    {
      ++leaves;
      EXPECT_FALSE(s->Shape().IsNull());
    }
  }
  EXPECT_EQ(groups, 2u);
  EXPECT_EQ(leaves, 4u);
  EXPECT_FALSE(view().find_shape_by_id(gid).IsNull());

  // Both groups remain document roots (sibling paste, not nested under the source).
  const std::vector<Shp_ptr> roots = view().shape_children(0);
  EXPECT_EQ(roots.size(), 2u);
  EXPECT_EQ(view().get_shapes().size(), 6u);

  const Shape_id pasted_gid = view().current_group_id();
  EXPECT_NE(pasted_gid, 0u);
  EXPECT_NE(pasted_gid, gid);
  Shp_ptr pasted = view().find_shape_by_id(pasted_gid);
  ASSERT_FALSE(pasted.IsNull());
  EXPECT_TRUE(pasted->is_group());
  EXPECT_EQ(pasted->get_parent_id(), 0u);
  EXPECT_EQ(view().shape_descendant_solids(pasted_gid).size(), 2u);
  EXPECT_EQ(view().shape_descendant_solids(gid).size(), 2u);
}

TEST_F(Shp_test, New_file_keeps_shape_clipboard)
{
  view().add_box(0, 0, 0, 1, 1, 1);
  select_shapes(view(), {view().get_shapes().back()});
  ASSERT_TRUE(view().copy_selected_shapes().is_ok());
  EXPECT_TRUE(view().has_shape_clipboard());
  view().new_file();
  EXPECT_TRUE(view().has_shape_clipboard());
  ASSERT_TRUE(view().paste_clipboard_shapes().is_ok());

  size_t leaves = 0;
  for (const Shp_ptr& s : view().get_shapes())
    if (!s->is_group())
      ++leaves;

  EXPECT_EQ(leaves, 1u);
}

TEST_F(Shp_test, Grouped_solids_stay_displayed_and_selectable)
{
  view().add_box(0, 0, 0, 1, 1, 1);
  view().add_box(3, 0, 0, 1, 1, 1);

  std::vector<Shp_ptr> boxes;
  for (const Shp_ptr& s : view().get_shapes())
    if (!s.IsNull() && !s->is_group())
      boxes.push_back(s);

  ASSERT_EQ(boxes.size(), 2u);
  ASSERT_TRUE(view().group_shapes(boxes).is_ok());

  AIS_InteractiveContext& ctx = view().ctx();
  for (const Shp_ptr& s : boxes)
  {
    EXPECT_NE(s->get_parent_id(), 0u);
    EXPECT_TRUE(ctx.IsDisplayed(s));
    NCollection_List<int> modes;
    ctx.ActivatedModes(s, modes);
    bool has_shape_mode = false;
    for (NCollection_List<int>::Iterator it(modes); it.More(); it.Next())
      if (it.Value() == AIS_Shape::SelectionMode(TopAbs_SHAPE))
        has_shape_mode = true;

    EXPECT_TRUE(has_shape_mode) << "id=" << s->get_id();

    ctx.ClearSelected(true);
    ctx.AddOrRemoveSelected(s, true);
    EXPECT_EQ(view().get_selected_shps().size(), 1u);
    EXPECT_EQ(view().get_selected_shps().front()->get_id(), s->get_id());
  }
}

TEST_F(Shp_test, Transform_axes_world_uses_bbox_center)
{
  view().add_box(0, 0, 0, 10, 10, 10);
  Shp_ptr shp = view().get_shapes().back();
  ASSERT_FALSE(shp.IsNull());

  const Transform_axes axes = transform_axes_for({shp}, Transform_space::World);
  EXPECT_TRUE(axes.origin.IsEqual(gp_Pnt(5.0, 5.0, 5.0), 1e-9));
  EXPECT_TRUE(axes.x.IsEqual(gp_Dir(1.0, 0.0, 0.0), 1e-9));
  EXPECT_TRUE(axes.z.IsEqual(gp_Dir(0.0, 0.0, 1.0), 1e-9));
}

TEST_F(Shp_test, Transform_axes_local_uses_assigned_frame)
{
  view().add_box(0, 0, 0, 10, 10, 10);
  Shp_ptr shp = view().get_shapes().back();
  ASSERT_FALSE(shp.IsNull());

  const gp_Ax3 tilted(gp_Pnt(1.0, 2.0, 3.0), gp_Dir(0.0, 1.0, 0.0), gp_Dir(1.0, 0.0, 0.0));
  view().set_shape_frame(shp, tilted);

  const Transform_axes local = transform_axes_for({shp}, Transform_space::Local);
  EXPECT_TRUE(local.origin.IsEqual(gp_Pnt(1.0, 2.0, 3.0), 1e-9));
  EXPECT_TRUE(local.x.IsEqual(gp_Dir(1.0, 0.0, 0.0), 1e-9));
  EXPECT_TRUE(local.z.IsEqual(gp_Dir(0.0, 1.0, 0.0), 1e-9));

  const Transform_axes world = transform_axes_for({shp}, Transform_space::World);
  EXPECT_TRUE(world.origin.IsEqual(gp_Pnt(5.0, 5.0, 5.0), 1e-9));
  EXPECT_TRUE(world.z.IsEqual(gp_Dir(0.0, 0.0, 1.0), 1e-9));
}

TEST_F(Shp_test, Transform_translation_local_x_constraint)
{
  Transform_axes axes;
  axes.origin = gp_Pnt(0.0, 0.0, 0.0);
  axes.x      = gp_Dir(0.0, 1.0, 0.0);
  axes.y      = gp_Dir(0.0, 0.0, 1.0);
  axes.z      = gp_Dir(1.0, 0.0, 0.0);

  const gp_Vec mouse(1.0, 4.0, 0.0);
  const gp_Vec delta =
      transform_translation(axes, mouse, true, false, false, std::nullopt, std::nullopt, std::nullopt);
  EXPECT_NEAR(delta.X(), 0.0, 1e-9);
  EXPECT_NEAR(delta.Y(), 4.0, 1e-9);
  EXPECT_NEAR(delta.Z(), 0.0, 1e-9);
}

TEST_F(Shp_test, Rotate_axis_can_be_set_before_first_drag)
{
  gui().set_mode(Mode::Design_inspection);
  gui().set_hide_all_shapes(false);
  view().add_box(0, 0, 0, 10, 10, 10);
  Shp_ptr shp = view().get_shapes().back();
  ASSERT_FALSE(shp.IsNull());
  select_shapes(view(), {shp});

  gui().set_mode(Mode::Workbench_rotate);
  EXPECT_TRUE(view().shp_rotate().has_operation_shps());
  view().shp_rotate().set_rotation_axis(Rotation_axis::Z_axis);
  EXPECT_EQ(view().shp_rotate().get_rotation_axis(), Rotation_axis::Z_axis);
}

TEST_F(Shp_test, Rotate_view_to_object_keeps_drag_frame_after_orbit)
{
  gui().set_mode(Mode::Design_inspection);
  gui().set_hide_all_shapes(false);
  view().add_box(0, 0, 0, 10, 10, 10);
  Shp_ptr shp = view().get_shapes().back();
  ASSERT_FALSE(shp.IsNull());
  select_shapes(view(), {shp});

  gui().set_mode(Mode::Workbench_rotate);
  ASSERT_TRUE(view().shp_rotate().has_operation_shps());
  EXPECT_EQ(view().shp_rotate().get_rotation_axis(), Rotation_axis::View_to_object);

  view().view_handle()->SetProj(0, 0, 1);
  Shp_rotate_access::capture_drag_frame(view().shp_rotate());
  ASSERT_TRUE(Shp_rotate_access::rotate_pln(view().shp_rotate()).has_value());
  ASSERT_TRUE(Shp_rotate_access::captured_axis_dir(view().shp_rotate()).has_value());

  const gp_Dir frozen_axis = *Shp_rotate_access::captured_axis_dir(view().shp_rotate());
  const gp_Dir frozen_pln  = Shp_rotate_access::rotate_pln(view().shp_rotate())->Axis().Direction();

  view().view_handle()->SetProj(1, 0, 0);
  const gp_Dir live_after = view().get_view_plane(gp_Pnt(5.0, 5.0, 5.0)).Axis().Direction();
  ASSERT_FALSE(live_after.IsEqual(frozen_axis, 1e-3));

  Shp_rotate_access::capture_drag_frame(view().shp_rotate());
  EXPECT_TRUE(Shp_rotate_access::captured_axis_dir(view().shp_rotate())->IsEqual(frozen_axis, 1e-9));
  EXPECT_TRUE(Shp_rotate_access::rotate_pln(view().shp_rotate())->Axis().Direction().IsEqual(frozen_pln, 1e-9));
}

TEST_F(Shp_test, Rotate_constrained_keeps_axis_plane_when_facing_test_would_flip)
{
  gui().set_mode(Mode::Design_inspection);
  gui().set_hide_all_shapes(false);
  view().add_box(0, 0, 0, 10, 10, 10);
  Shp_ptr shp = view().get_shapes().back();
  ASSERT_FALSE(shp.IsNull());
  select_shapes(view(), {shp});

  gui().set_mode(Mode::Workbench_rotate);
  ASSERT_TRUE(view().shp_rotate().has_operation_shps());
  view().shp_rotate().set_rotation_axis(Rotation_axis::X_axis);

  // Look along +X so the view faces the rotation axis (dot ~ 1) and the axis plane is used.
  view().view_handle()->SetProj(1, 0, 0);
  Shp_rotate_access::capture_drag_frame(view().shp_rotate());
  ASSERT_TRUE(Shp_rotate_access::rotate_pln(view().shp_rotate()).has_value());
  EXPECT_TRUE(Shp_rotate_access::rotate_pln(view().shp_rotate())->Axis().Direction().IsEqual(gp_Dir(1.0, 0.0, 0.0), 1e-6));

  // Edge-on to X: live facing test would fall back to the view plane (normal ~ Z).
  view().view_handle()->SetProj(0, 0, 1);
  const gp_Dir live_view = view().get_view_plane(gp_Pnt(5.0, 5.0, 5.0)).Axis().Direction();
  ASSERT_LT(std::abs(live_view.Dot(gp_Dir(1.0, 0.0, 0.0))), 0.15);

  Shp_rotate_access::capture_drag_frame(view().shp_rotate());
  EXPECT_TRUE(Shp_rotate_access::rotate_pln(view().shp_rotate())->Axis().Direction().IsEqual(gp_Dir(1.0, 0.0, 0.0), 1e-6));
}

TEST_F(Shp_test, Scale_space_change_mid_drag_keeps_factor)
{
  gui().set_mode(Mode::Design_inspection);
  gui().set_hide_all_shapes(false);
  const Transform_space saved_space = gui().get_transform_space();
  struct Restore_space
  {
    GUI&            gui;
    Transform_space saved;
    ~Restore_space() { GUI_access::set_transform_space(gui, saved); }
  } restore{gui(), saved_space};
  GUI_access::set_transform_space(gui(), Transform_space::Local);

  view().add_box(0, 0, 0, 10, 10, 10);
  Shp_ptr shp = view().get_shapes().back();
  ASSERT_FALSE(shp.IsNull());
  view().set_shape_frame(shp, gp_Ax3(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0), gp_Dir(1.0, 0.0, 0.0)));
  select_shapes(view(), {shp});

  gui().set_mode(Mode::Scale);
  ASSERT_TRUE(view().shp_scale().has_operation_shps());
  ASSERT_TRUE(Shp_scale_access::ensure_start(view().shp_scale()).is_ok());
  ASSERT_TRUE(Shp_scale_access::center(view().shp_scale())->IsEqual(gp_Pnt(0.0, 0.0, 0.0), 1e-6));

  ASSERT_TRUE(Shp_scale_access::apply_distance(view().shp_scale(), 20.0).is_ok());
  EXPECT_NEAR(Shp_scale_access::scale_factor(view().shp_scale()), 1.0, 1e-9);

  ASSERT_TRUE(Shp_scale_access::apply_distance(view().shp_scale(), 40.0).is_ok());
  const double factor_before = Shp_scale_access::scale_factor(view().shp_scale());
  EXPECT_NEAR(factor_before, 2.0, 1e-9);

  GUI_access::set_transform_space(gui(), Transform_space::World);
  ASSERT_TRUE(Shp_scale_access::center(view().shp_scale()).has_value());
  EXPECT_TRUE(Shp_scale_access::center(view().shp_scale())->IsEqual(gp_Pnt(5.0, 5.0, 5.0), 1e-6));
  EXPECT_NEAR(Shp_scale_access::scale_factor(view().shp_scale()), factor_before, 1e-9);
  EXPECT_LT(Shp_scale_access::initial_distance(view().shp_scale()), 1e-9);

  const double dist_after = gp_Pnt(5.0, 5.0, 5.0).Distance(gp_Pnt(40.0, 0.0, 0.0));
  ASSERT_TRUE(Shp_scale_access::apply_distance(view().shp_scale(), dist_after).is_ok());
  EXPECT_NEAR(Shp_scale_access::scale_factor(view().shp_scale()), factor_before, 1e-6);

  gui().set_mode(Mode::Design_inspection);
}

TEST_F(Shp_test, Rotate_space_change_mid_drag_keeps_angle)
{
  gui().set_mode(Mode::Design_inspection);
  gui().set_hide_all_shapes(false);
  const Transform_space saved_space = gui().get_transform_space();
  struct Restore_space
  {
    GUI&            gui;
    Transform_space saved;
    ~Restore_space() { GUI_access::set_transform_space(gui, saved); }
  } restore{gui(), saved_space};
  GUI_access::set_transform_space(gui(), Transform_space::Local);

  view().add_box(0, 0, 0, 10, 10, 10);
  Shp_ptr shp = view().get_shapes().back();
  ASSERT_FALSE(shp.IsNull());
  view().set_shape_frame(shp, gp_Ax3(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0), gp_Dir(1.0, 0.0, 0.0)));
  select_shapes(view(), {shp});

  gui().set_mode(Mode::Workbench_rotate);
  ASSERT_TRUE(view().shp_rotate().has_operation_shps());
  view().shp_rotate().set_rotation_axis(Rotation_axis::Z_axis);
  ASSERT_TRUE(Shp_rotate_access::ensure_start(view().shp_rotate()).is_ok());
  ASSERT_TRUE(Shp_rotate_access::center(view().shp_rotate())->IsEqual(gp_Pnt(0.0, 0.0, 0.0), 1e-6));

  const gp_Dir axis_z(0.0, 0.0, 1.0);
  const gp_Pln local_pln(gp_Pnt(0.0, 0.0, 0.0), axis_z);
  ASSERT_TRUE(Shp_rotate_access::apply_world(view().shp_rotate(), gp_Pnt(20.0, 0.0, 0.0), axis_z, local_pln).is_ok());
  ASSERT_TRUE(Shp_rotate_access::apply_world(view().shp_rotate(), gp_Pnt(0.0, 20.0, 0.0), axis_z, local_pln).is_ok());
  const double angle_before = Shp_rotate_access::angle(view().shp_rotate());
  EXPECT_NEAR(angle_before, std::numbers::pi / 2.0, 1e-6);
  ASSERT_TRUE(Shp_rotate_access::initial_mouse_pos(view().shp_rotate()).has_value());

  GUI_access::set_transform_space(gui(), Transform_space::World);
  ASSERT_TRUE(Shp_rotate_access::center(view().shp_rotate()).has_value());
  EXPECT_TRUE(Shp_rotate_access::center(view().shp_rotate())->IsEqual(gp_Pnt(5.0, 5.0, 5.0), 1e-6));
  EXPECT_NEAR(Shp_rotate_access::angle(view().shp_rotate()), angle_before, 1e-9);
  EXPECT_FALSE(Shp_rotate_access::initial_mouse_pos(view().shp_rotate()).has_value());

  const gp_Pln world_pln(gp_Pnt(5.0, 5.0, 5.0), axis_z);
  ASSERT_TRUE(Shp_rotate_access::apply_world(view().shp_rotate(), gp_Pnt(0.0, 20.0, 0.0), axis_z, world_pln).is_ok());
  EXPECT_NEAR(Shp_rotate_access::angle(view().shp_rotate()), angle_before, 1e-6);

  gui().set_mode(Mode::Design_inspection);
}

// ---------------------------------------------------------------------------
// Workbench list (geometry links, own placement)
// ---------------------------------------------------------------------------

TEST(Mode_helpers, Design_move_rotate_stay_on_design_task)
{
  EXPECT_EQ(task_of(Mode::Design_move), Task::Design);
  EXPECT_EQ(task_of(Mode::Design_rotate), Task::Design);
  EXPECT_EQ(task_of(Mode::Design_shaft_align), Task::Design);
  EXPECT_EQ(task_of(Mode::Scale), Task::Design);
  EXPECT_EQ(task_of(Mode::Sketch_face_extrude), Task::Sketch);
  EXPECT_EQ(GUI::parent_mode_of(Mode::Sketch_face_extrude), Mode::Sketch_inspection);
  EXPECT_EQ(task_of(Mode::Workbench_move), Task::Workbench);
  EXPECT_EQ(task_of(Mode::Workbench_rotate), Task::Workbench);
  EXPECT_EQ(task_of(Mode::Workbench_shaft_align), Task::Workbench);
  EXPECT_TRUE(is_move_mode(Mode::Design_move));
  EXPECT_TRUE(is_rotate_mode(Mode::Design_rotate));
  EXPECT_TRUE(is_shaft_align_mode(Mode::Design_shaft_align));
  EXPECT_TRUE(is_shaft_align_mode(Mode::Workbench_shaft_align));
  EXPECT_FALSE(is_workbench_mode(Mode::Design_move));
  EXPECT_FALSE(is_workbench_mode(Mode::Design_rotate));
  EXPECT_FALSE(is_workbench_mode(Mode::Design_shaft_align));
  EXPECT_EQ(GUI::parent_mode_of(Mode::Design_move), Mode::Design_inspection);
  EXPECT_EQ(GUI::parent_mode_of(Mode::Design_rotate), Mode::Design_inspection);
  EXPECT_EQ(GUI::parent_mode_of(Mode::Design_shaft_align), Mode::Design_inspection);
  EXPECT_EQ(GUI::parent_mode_of(Mode::Workbench_move), Mode::Workbench_inspection);
  EXPECT_EQ(GUI::parent_mode_of(Mode::Workbench_shaft_align), Mode::Workbench_inspection);
  EXPECT_EQ(task_of(Mode::Workbench_set_frame), Task::Workbench);
  EXPECT_TRUE(is_set_frame_mode(Mode::Workbench_set_frame));
  EXPECT_TRUE(is_workbench_mode(Mode::Workbench_set_frame));
  EXPECT_EQ(GUI::parent_mode_of(Mode::Workbench_set_frame), Mode::Workbench_inspection);
  EXPECT_EQ(mode_from_string("Move"), Mode::Design_move);
  EXPECT_EQ(mode_from_string("Rotate"), Mode::Design_rotate);
  EXPECT_EQ(mode_from_string("Shape_shaft_align"), Mode::Design_shaft_align);
  EXPECT_EQ(static_cast<int>(Mode::Design_move), 1);
  EXPECT_EQ(static_cast<int>(Mode::Design_rotate), 3);
  EXPECT_EQ(static_cast<int>(Mode::Design_shaft_align), 24);
  EXPECT_NE(static_cast<int>(Mode::Workbench_move), static_cast<int>(Mode::Design_move));
  EXPECT_NE(static_cast<int>(Mode::Workbench_rotate), static_cast<int>(Mode::Design_rotate));
  EXPECT_NE(static_cast<int>(Mode::Workbench_shaft_align), static_cast<int>(Mode::Design_shaft_align));
}

TEST(Shp_frame, Trsf_from_frame_maps_local_origin_to_world)
{
  const gp_Ax3 f(gp_Pnt(10.0, 20.0, 30.0), gp_Dir(0.0, 0.0, 1.0), gp_Dir(1.0, 0.0, 0.0));
  const gp_Pnt world = gp_Pnt(0.0, 0.0, 0.0).Transformed(Shp::trsf_from_frame(f));
  EXPECT_TRUE(world.IsEqual(gp_Pnt(10.0, 20.0, 30.0), 1e-9));
}

TEST_F(Shp_test, Add_to_workbench_creates_geometry_link)
{
  view().add_box(0, 0, 0, 10, 10, 10);
  ASSERT_EQ(view().get_shapes().size(), 1u);
  Shp_ptr src = view().get_shapes().back();
  const Shape_id src_id = src->get_id();
  const double   vol    = volume_of(src->Shape());

  ASSERT_TRUE(view().add_to_workbench({src}).is_ok());
  ASSERT_EQ(view().get_workbench_shapes().size(), 1u);
  Shp_ptr inst = view().get_workbench_shapes().back();
  EXPECT_TRUE(inst->is_workbench());
  EXPECT_TRUE(inst->is_workbench_link());
  EXPECT_EQ(inst->get_source_id(), src_id);
  EXPECT_NE(inst->get_id(), src_id);
  EXPECT_NEAR(volume_of(inst->Shape()), vol, 1e-6);
}

TEST_F(Shp_test, Design_move_does_not_move_workbench_instance)
{
  view().add_box(0, 0, 0, 10, 10, 10);
  Shp_ptr src = view().get_shapes().back();
  ASSERT_TRUE(view().add_to_workbench({src}).is_ok());
  Shp_ptr inst = view().get_workbench_shapes().back();
  const gp_Pnt inst_origin = inst->get_frame().Location();
  const gp_Pnt src_origin  = src->get_frame().Location();

  gp_Trsf move;
  move.SetTranslation(gp_Vec(40.0, 0.0, 0.0));
  src->SetLocalTransformation(move);
  AIS_Shape_ptr ais = src;
  view().bake_transform_into_geometry(ais);
  view().sync_workbench_links(src->get_id());

  EXPECT_TRUE(src->get_frame().Location().IsEqual(src_origin.Translated(gp_Vec(40.0, 0.0, 0.0)), 1e-6));
  EXPECT_TRUE(inst->get_frame().Location().IsEqual(inst_origin, 1e-6));
  EXPECT_NEAR(volume_of(inst->Shape()), volume_of(src->Shape()), 1e-6);
}

TEST_F(Shp_test, Workbench_move_bakes_instance_frame_only)
{
  view().add_box(0, 0, 0, 10, 10, 10);
  Shp_ptr src = view().get_shapes().back();
  ASSERT_TRUE(view().add_to_workbench({src}).is_ok());
  Shp_ptr inst = view().get_workbench_shapes().back();
  const gp_Pnt frame0 = inst->get_frame().Location();
  const gp_Pnt disp0  = get_shape_bbox_center(inst->Shape()).Transformed(inst->placement_trsf());

  gp_Trsf move;
  move.SetTranslation(gp_Vec(25.0, 0.0, 0.0));
  inst->SetLocalTransformation(move * inst->placement_trsf());
  AIS_Shape_ptr ais = inst;
  view().bake_transform_into_geometry(ais);

  EXPECT_TRUE(inst->get_frame().Location().IsEqual(frame0.Translated(gp_Vec(25.0, 0.0, 0.0)), 1e-6));
  const gp_Pnt disp1 = get_shape_bbox_center(inst->Shape()).Transformed(inst->placement_trsf());
  EXPECT_TRUE(disp1.IsEqual(disp0.Translated(gp_Vec(25.0, 0.0, 0.0)), 1e-6));
  EXPECT_NEAR(volume_of(inst->Shape()), volume_of(src->Shape()), 1e-6);

  const Transform_axes world_axes = transform_axes_for({inst}, Transform_space::World);
  EXPECT_TRUE(world_axes.origin.IsEqual(disp1, 1e-6));
}

TEST_F(Shp_test, Workbench_cyl_align_uses_instance_placement)
{
  view().add_cylinder(0, 0, 0, 1.0, 4.0);
  Shp_ptr src = view().get_shapes().back();
  ASSERT_TRUE(view().add_to_workbench({src}).is_ok());
  ASSERT_TRUE(view().add_to_workbench({src}).is_ok());
  auto it = view().get_workbench_shapes().begin();
  Shp_ptr moving = *it++;
  Shp_ptr fixed  = *it;
  ASSERT_FALSE(moving.IsNull());
  ASSERT_FALSE(fixed.IsNull());

  gp_Ax3 placed = moving->get_frame();
  placed.SetLocation(placed.Location().Translated(gp_Vec(50.0, 0.0, 0.0)));
  moving->set_frame(placed);
  moving->SetLocalTransformation(moving->placement_trsf());

  auto first_cyl = [](const TopoDS_Shape& s) -> std::optional<Cyl_face_info>
  {
    for (TopExp_Explorer ex(s, TopAbs_FACE); ex.More(); ex.Next())
      if (std::optional<Cyl_face_info> c = cylinder_from_face(TopoDS::Face(ex.Current())))
        return c;

    return std::nullopt;
  };

  const std::optional<Cyl_face_info> moving_local = first_cyl(moving->Shape());
  const std::optional<Cyl_face_info> fixed_local  = first_cyl(fixed->Shape());
  ASSERT_TRUE(moving_local.has_value());
  ASSERT_TRUE(fixed_local.has_value());

  const gp_Ax1 moving_world = moving_local->axis.Transformed(moving->LocalTransformation());
  const gp_Ax1 fixed_world  = fixed_local->axis.Transformed(fixed->LocalTransformation());
  const gp_Trsf local_only  = cyl_align_trsf(moving_local->axis, fixed_local->axis, false, 0.0, 0.0);
  const gp_Trsf world_align = cyl_align_trsf(moving_world, fixed_world, false, 0.0, 0.0);

  // Same linked local geom: local-only align does not close the instance gap.
  EXPECT_LT(local_only.TranslationPart().Modulus(), 1.0);
  EXPECT_GT(world_align.TranslationPart().Modulus(), 40.0);

  const gp_Pnt after = moving_world.Location().Transformed(world_align);
  EXPECT_NEAR(gp_Lin(fixed_world).Distance(after), 0.0, 1e-6);
}

TEST(Shp_cyl_align, Prefers_smaller_rotation)
{
  const gp_Ax1 fixed(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0));
  const gp_Ax1 anti(gp_Pnt(8.0, 0.0, 0.0), gp_Dir(0.0, 0.0, -1.0));

  const gp_Trsf keep = cyl_align_trsf(anti, fixed, false, 0.0, 0.0);
  EXPECT_GT(gp_Vec(0.0, 0.0, 1.0).Transformed(keep).Z(), 0.9);

  const gp_Trsf flipped = cyl_align_trsf(anti, fixed, true, 0.0, 0.0);
  EXPECT_LT(gp_Vec(0.0, 0.0, 1.0).Transformed(flipped).Z(), -0.9);

  const gp_Ax1  same(gp_Pnt(8.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0));
  const gp_Trsf par = cyl_align_trsf(same, fixed, false, 0.0, 0.0);
  EXPECT_GT(gp_Vec(0.0, 0.0, 1.0).Transformed(par).Z(), 0.9);
}

TEST_F(Shp_test, Workbench_set_frame_keeps_instance_pose)
{
  view().add_cylinder(0, 0, 0, 1.0, 4.0);
  Shp_ptr src = view().get_shapes().back();
  ASSERT_TRUE(view().add_to_workbench({src}).is_ok());
  Shp_ptr inst = view().get_workbench_shapes().back();
  const gp_Pnt pose0 = inst->get_frame().Location();

  const gp_Ax3 local(gp_Pnt(0.0, 0.0, 1.0), gp_Dir(0.0, 0.0, 1.0), gp_Dir(1.0, 0.0, 0.0));
  view().set_shape_frame(inst, local);

  EXPECT_TRUE(inst->get_frame().Location().IsEqual(pose0, 1e-9));
  EXPECT_TRUE(inst->get_local_frame().Location().IsEqual(local.Location(), 1e-9));
  EXPECT_TRUE(inst->get_local_frame().Direction().IsEqual(local.Direction(), 1e-9));
}

TEST_F(Shp_test, Design_geom_change_syncs_workbench_link)
{
  view().add_box(0, 0, 0, 10, 10, 10);
  Shp_ptr src = view().get_shapes().back();
  ASSERT_TRUE(view().add_to_workbench({src}).is_ok());
  Shp_ptr inst = view().get_workbench_shapes().back();
  EXPECT_NEAR(volume_of(inst->Shape()), 1000.0, 1e-4);

  const TopoDS_Shape bigger = shp_create::create_box(0, 0, 0, 20, 20, 20);
  view().set_shape_geom_by_id(src->get_id(), bigger, src->get_frame());
  EXPECT_NEAR(volume_of(inst->Shape()), 8000.0, 1e-3);
  EXPECT_TRUE(inst->get_frame().Location().IsEqual(src->get_frame().Location(), 1e-6));
}

TEST_F(Shp_test, Workbench_json_round_trip_and_source_delete)
{
  view().add_box(0, 0, 0, 4, 5, 6);
  Shp_ptr src = view().get_shapes().back();
  const Shape_id src_id = src->get_id();
  ASSERT_TRUE(view().add_to_workbench({src}).is_ok());
  Shp_ptr inst = view().get_workbench_shapes().back();
  const Shape_id inst_id = inst->get_id();
  gp_Ax3 placed = inst->get_frame();
  placed.SetLocation(gp_Pnt(15.0, 0.0, 0.0));
  inst->set_frame(placed);

  const std::string json = view().to_json();
  view().new_file();
  EXPECT_TRUE(view().get_shapes().empty());
  EXPECT_TRUE(view().get_workbench_shapes().empty());

  view().load(json, false);
  ASSERT_EQ(view().get_shapes().size(), 1u);
  ASSERT_EQ(view().get_workbench_shapes().size(), 1u);
  Shp_ptr loaded = view().get_workbench_shapes().back();
  EXPECT_EQ(loaded->get_id(), inst_id);
  EXPECT_EQ(loaded->get_source_id(), src_id);
  EXPECT_TRUE(loaded->is_workbench_link());
  EXPECT_TRUE(loaded->get_frame().Location().IsEqual(gp_Pnt(15.0, 0.0, 0.0), 1e-6));
  EXPECT_NEAR(volume_of(loaded->Shape()), volume_of(view().get_shapes().back()->Shape()), 1e-6);

  view().delete_shapes({view().get_shapes().back()});
  EXPECT_TRUE(view().get_shapes().empty());
  EXPECT_TRUE(view().get_workbench_shapes().empty());
}

TEST_F(Shp_test, Undo_delete_design_restores_workbench_links)
{
  view().add_box(0, 0, 0, 4, 5, 6);
  Shp_ptr        src    = view().get_shapes().back();
  const Shape_id src_id = src->get_id();
  ASSERT_TRUE(view().add_to_workbench({src}).is_ok());
  ASSERT_TRUE(view().add_to_workbench({src}).is_ok());
  ASSERT_EQ(view().get_workbench_shapes().size(), 2u);
  const Shape_id inst_a = view().get_workbench_shapes().front()->get_id();
  const Shape_id inst_b = view().get_workbench_shapes().back()->get_id();

  view().delete_shapes({src});
  EXPECT_TRUE(view().get_shapes().empty());
  EXPECT_TRUE(view().get_workbench_shapes().empty());

  EXPECT_TRUE(view().undo());
  ASSERT_EQ(view().get_shapes().size(), 1u);
  EXPECT_EQ(view().get_shapes().back()->get_id(), src_id);
  ASSERT_EQ(view().get_workbench_shapes().size(), 2u);
  EXPECT_EQ(view().get_workbench_shapes().front()->get_id(), inst_a);
  EXPECT_EQ(view().get_workbench_shapes().back()->get_id(), inst_b);
  for (const Shp_ptr& inst : view().get_workbench_shapes())
  {
    EXPECT_EQ(inst->get_source_id(), src_id);
    EXPECT_NEAR(volume_of(inst->Shape()), volume_of(view().get_shapes().back()->Shape()), 1e-6);
  }

  EXPECT_TRUE(view().redo());
  EXPECT_TRUE(view().get_shapes().empty());
  EXPECT_TRUE(view().get_workbench_shapes().empty());
}

TEST_F(Shp_test, Cut_rejects_workbench_instances)
{
  view().add_box(0, 0, 0, 10, 10, 10);
  view().add_box(0, 0, 0, 5, 10, 10);
  std::vector<Shp_ptr> design(view().get_shapes().begin(), view().get_shapes().end());
  ASSERT_TRUE(view().add_to_workbench({design[0]}).is_ok());
  ASSERT_TRUE(view().add_to_workbench({design[1]}).is_ok());
  std::vector<Shp_ptr> instances(view().get_workbench_shapes().begin(), view().get_workbench_shapes().end());
  select_shapes(view(), instances);

  Status st = view().shp_cut().selected_cut();
  EXPECT_FALSE(st.is_ok());
  EXPECT_EQ(view().get_shapes().size(), 2u);
  EXPECT_EQ(view().get_workbench_shapes().size(), 2u);
  EXPECT_EQ(view().get_shapes().front()->get_id(), design[0]->get_id());
  EXPECT_EQ(view().get_workbench_shapes().front()->get_id(), instances[0]->get_id());
}

TEST_F(Shp_test, Add_design_group_to_workbench)
{
  view().add_box(0, 0, 0, 1, 1, 1);
  view().add_box(3, 0, 0, 1, 1, 1);
  std::vector<Shp_ptr> boxes(view().get_shapes().begin(), view().get_shapes().end());
  ASSERT_TRUE(view().group_shapes(boxes).is_ok());
  Shp_ptr grp;
  for (const Shp_ptr& s : view().get_shapes())
    if (s->is_group())
      grp = s;

  ASSERT_FALSE(grp.IsNull());
  ASSERT_TRUE(view().add_to_workbench({grp}).is_ok());
  EXPECT_EQ(view().get_workbench_shapes().size(), 3u);

  size_t wbk_groups = 0;
  size_t wbk_links  = 0;
  for (const Shp_ptr& s : view().get_workbench_shapes())
  {
    if (s->is_group())
      ++wbk_groups;
    else if (s->is_workbench_link())
      ++wbk_links;
  }

  EXPECT_EQ(wbk_groups, 1u);
  EXPECT_EQ(wbk_links, 2u);
}
