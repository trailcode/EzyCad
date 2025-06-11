#include "shp_rotate.h"
#include "geom.h"
#include "gui.h"
#include "occt_view.h"
#include "imgui.h"

Shp_rotate::Shp_rotate(Occt_view& view)
    : Shp_operation_base(view) {}

Status Shp_rotate::rotate_selected(const ScreenCoords& screen_coords)
{
  std::vector<AIS_Shape_ptr> to_rotate = get_selected();
  if (to_rotate.empty())
    return Status::user_error("Select one or more shapes.");

  if (!m_start.has_value())
    m_start = screen_coords;

  // Use the point of the first selected object.
  gp_Pnt center = get_shape_bbox_center(to_rotate.front()->Shape());

  double width;
  double height;
  //view().get_occt_view()->Size(width, height);
  
  width = ImGui::GetIO().DisplaySize.x;
  height = ImGui::GetIO().DisplaySize.y;
  double  radius = 0.5;
  gp_Vec  v0    = map_to_sphere(*m_start, center, radius, width, height);
  gp_Vec  v1    = map_to_sphere(screen_coords, center, radius, width, height);
  if (v0.IsEqual(v1, Precision::Confusion(), Precision::Angular()))
    return Status::ok();

  //gp_Vec  axis  = v0.Crossed(v1);
  gp_Vec  axis  = v0.Crossed(v1);
  double  angle = acos(v0.Dot(v1));

  //DBG_MSG(axis.XYZ().X() << "," << axis.XYZ().Y() << "," << axis.XYZ().Z());
  DBG_MSG(v1.XYZ().X() << "," << v1.XYZ().Y() << "," << v1.XYZ().Z());

  //gp_Ax1  rotation_axis(center, axis);
  gp_Ax1  rotation_axis(gp_Pnt(0, 0, 0), axis);
  gp_Trsf rotation;
  rotation.SetRotation(rotation_axis, angle);
  // Apply to shape
  //shape->SetLocalTransformation(rotation * shape->LocalTransformation());

  for (const AIS_Shape_ptr& shape : to_rotate)
  {
    //gp_Trsf translation;
    //translation.SetTranslation(gp_Vec(m_delta.delta));

    shape->SetLocalTransformation(rotation);
    ctx().Redisplay(shape, true);
  }

  return Status::ok();
}

void Shp_rotate::show_dist_edit(const ScreenCoords& screen_coords)
{

}

void Shp_rotate::finalize_rotate_selected()
{

}

void Shp_rotate::cancel_rotate_selected()
{

}