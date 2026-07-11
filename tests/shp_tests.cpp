#include "sketch_test_fixture.h"

#include <AIS_InteractiveContext.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <numbers>

#include "shp.h"
#include "shp_create.h"
#include "shp_info.h"
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
  const double         r      = 2.0;
  const TopoDS_Shape   sphere = shp_create::create_sphere(r);
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
// shp_info
// ---------------------------------------------------------------------------

TEST(Shp_info, Collect_null_shape)
{
  const auto lines = shp_info::collect(TopoDS_Shape());
  EXPECT_EQ(line_value(lines, "Shape"), "null");
}

TEST(Shp_info, Collect_box_with_display_meta)
{
  const TopoDS_Shape box = shp_create::create_box(0, 0, 0, 2, 3, 4);
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
