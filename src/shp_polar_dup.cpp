#include "shp_polar_dup.h"
#include "geom.h"
#include "modes.h"
#include "occt_view.h"
#include "sketch.h"
#include "sketch_nodes.h"
#include "gui.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <numbers>

Shp_polar_dup::Shp_polar_dup(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_polar_dup::move_point(const ScreenCoords& screen_coords)
{
  if (m_polar_arm_origin.has_value())
    return Status::ok();

  const gp_Pln& sketch_pln = view().curr_sketch().get_plane();

  if (m_shps.empty())
  {
    CHK_RET(ensure_operation_shps_());

    // TODO consider all shapes.
    m_shps_center = get_shape_bbox_center(m_shps[0]->Shape());
  }

  EZY_ASSERT(m_shps.size());

  Sketch_nodes& nodes = view().curr_sketch().get_nodes();

  const std::optional<gp_Pnt2d> origin = nodes.snap(screen_coords);
  if (!origin.has_value())
    return Status::user_error("Error, rotate view, cannot get point on sketch plane.");

  const gp_Pnt2d start = to_2d(sketch_pln, m_shps_center);
  if (!unique(start, *origin))
    return Status::ok();

  TopoDS_Shape polar_arm = BRepBuilderAPI_MakeEdge(to_3d(sketch_pln, start), to_3d(sketch_pln, *origin)).Edge();
  if (m_polar_arm)
  {
    m_polar_arm->Set(polar_arm);
    ctx().Redisplay(m_polar_arm, true);
  }
  else
  {
    m_polar_arm = new AIS_Shape(polar_arm);
    ctx().Display(m_polar_arm, true);
  }

  return Status::ok();
}

Status Shp_polar_dup::add_point(const ScreenCoords& screen_coords)
{
  if (m_polar_arm_origin.has_value())
    return Status::ok();

  if (m_shps.empty())
    return Status::user_error("Select one or more shapes.");

  EZY_ASSERT(m_polar_arm);

  const TopoDS_Edge& edge                       = TopoDS::Edge(m_polar_arm->Shape());
  std::tie(m_polar_arm_end, m_polar_arm_origin) = get_edge_endpoints(view().curr_sketch().get_plane(), edge);

  return Status::ok();
}

Status Shp_polar_dup::dup()
{
  if (!m_polar_arm_end.has_value())
    return Status::user_error("Polar arm not set.");

  const double  step_angle = m_polar_angle / double(m_num_elms);
  const gp_Pln& sketch_pln = view().curr_sketch().get_plane();
  
  // Get the rotation center in 2D
  const gp_Pnt2d origin_2d = *m_polar_arm_origin;
  
  // Vector to store all transformed shapes if combining
  std::vector<TopoDS_Shape> transformed_shapes;
  
  // Create copies and rotate them
  for (size_t i = 0; i < m_num_elms; ++i)
  {
    const double current_angle_degrees = step_angle * (i + 1); // Skip 0 since that's the original
    const double current_angle_radians = current_angle_degrees * std::numbers::pi / 180.0;  // Convert to radians
    
    for (const AIS_Shape_ptr& shape : m_shps)
    {
      // Get the shape's center in 2D
      const gp_Pnt shape_center_3d = get_shape_bbox_center(shape->Shape());
      const gp_Pnt2d shape_center_2d = to_2d(sketch_pln, shape_center_3d);
      
      // Calculate the rotated shape center point in 2D
      const gp_Pnt2d rotated_shp_center_2d = rotate_point(origin_2d, shape_center_2d, current_angle_degrees);
      
      // Convert back to 3D
      const gp_Pnt rotated_shp_center_3d = to_3d(sketch_pln, rotated_shp_center_2d);
      
      // Create translation transformation
      gp_Trsf translation;
      translation.SetTranslation(gp_Vec(shape_center_3d, rotated_shp_center_3d));
      
      // Create rotation transformation if m_rotate_dups is true
      gp_Trsf combined;
      if (m_rotate_dups)
      {
        gp_Trsf rotation;
        rotation.SetRotation(gp_Ax1(to_3d(sketch_pln, *m_polar_arm_end), sketch_pln.Axis().Direction()), current_angle_radians);
        combined = translation * rotation;
      } else
        combined = translation;

      // Create a copy of the shape
      TopoDS_Shape shape_copy = shape->Shape();

      // Apply the transformation
      BRepBuilderAPI_Transform transformer(shape_copy, combined, true);
      const TopoDS_Shape& transformed_shape = transformer.Shape();
      
      if (m_combine_dups)
        // Store the transformed shape for later combination
        transformed_shapes.push_back(transformed_shape);
      else
      {
        // Create new shape and add to view as before
        ExtrudedShp_ptr new_shape = new ExtrudedShp(ctx(), transformed_shape);
        new_shape->set_name("Polar duplicate");
        add_shp_(new_shape);
      }
    }
  }

  if (m_combine_dups && !transformed_shapes.empty())
  {
    // Combine all transformed shapes into one
    TopoDS_Shape combined_shape = transformed_shapes[0];
    for (size_t i = 1; i < transformed_shapes.size(); ++i)
    {
      BRepAlgoAPI_Fuse fuse(combined_shape, transformed_shapes[i]);
      if (fuse.IsDone())
        combined_shape = fuse.Shape();
    }
    
    // Create a single shape from all the combined shapes
    ExtrudedShp_ptr new_shape = new ExtrudedShp(ctx(), combined_shape);
    new_shape->set_name("Combined polar duplicate");
    add_shp_(new_shape);
  }

  delete_operation_shps_();
  gui().set_mode(Mode::Normal); // Will call reset()
  return Status::ok();
}

void Shp_polar_dup::reset()
{
  if (m_polar_arm)
  {
    ctx().Remove(m_polar_arm, false);
    ctx().ClearSelected(true);
    clear_all(m_polar_arm, m_polar_arm_end, m_polar_arm_origin, m_shps);
  }
}

// TODO check values
// clang-format off
double Shp_polar_dup::get_polar_angle() const             { return m_polar_angle; }
void   Shp_polar_dup::set_polar_angle(const double angle) { m_polar_angle = angle; }
size_t Shp_polar_dup::get_num_elms() const                { return m_num_elms; }
void   Shp_polar_dup::set_num_selms(const size_t num)     { m_num_elms = num; }
bool   Shp_polar_dup::get_rotate_dups() const             { return m_rotate_dups; }
void   Shp_polar_dup::set_rotate_dups(const bool rotate)  { m_rotate_dups = rotate; }
bool   Shp_polar_dup::get_combine_dups() const            { return m_combine_dups; }
void   Shp_polar_dup::set_combine_dups(const bool combine) { m_combine_dups = combine; }
// clang-format on