#include "shp_set_frame.h"

#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <optional>

#include "gui.h"
#include "gui_occt_view.h"
#include "mode.h"
#include "utl_geom.h"

namespace
{
gp_Pnt bbox_center_(const TopoDS_Shape& shape)
{
  Bnd_Box bounds;
  BRepBndLib::Add(shape, bounds);
  if (bounds.IsVoid())
    return gp_Pnt();

  double x_min, y_min, z_min, x_max, y_max, z_max;
  bounds.Get(x_min, y_min, z_min, x_max, y_max, z_max);
  return gp_Pnt((x_min + x_max) * 0.5, (y_min + y_max) * 0.5, (z_min + z_max) * 0.5);
}

gp_Pnt project_onto_axis_(const gp_Ax1& axis, const gp_Pnt& p)
{
  const gp_Vec to_p(axis.Location(), p);
  const double t = to_p.Dot(gp_Vec(axis.Direction()));
  return axis.Location().Translated(gp_Vec(axis.Direction()) * t);
}

gp_Pnt project_onto_plane_(const gp_Pln& pln, const gp_Pnt& p)
{
  const gp_Vec n(pln.Axis().Direction());
  const gp_Vec v(pln.Location(), p);
  return p.Translated(-n * v.Dot(n));
}

/// Area centroid of a planar face (circle center for a disk); falls back to face AABB center.
gp_Pnt planar_face_origin_(const TopoDS_Face& face, const gp_Pln& pln)
{
  GProp_GProps props;
  BRepGProp::SurfaceProperties(face, props);
  if (props.Mass() > 0.0)
    return project_onto_plane_(pln, props.CentreOfMass());

  return project_onto_plane_(pln, bbox_center_(face));
}
} // namespace

Shp_set_frame::Shp_set_frame(Occt_view& view)
    : Shp_operation_base(view)
{
}

void Shp_set_frame::begin(const Shp_ptr& target, Pick pick)
{
  m_target = target;
  m_pick   = pick;
  if (m_target.IsNull() || m_target->is_group())
  {
    clear_all(m_target);
    return;
  }

  if (m_pick == Pick::Planar_face)
    gui().show_message("Pick a planar face to set the local frame (Z = normal).");
  else
    gui().show_message("Pick a cylindrical face to set the local frame (Z = axis).");
}

Status Shp_set_frame::pick(const ScreenCoords& screen_coords)
{
  if (m_target.IsNull() || m_target->is_group())
    return Status::user_error("No target shape for set frame.");

  Shp_ptr shp = Shp_ptr::DownCast(get_shape_(screen_coords));
  if (shp.IsNull() || shp != m_target)
    return Status::user_error("Pick a face on the selected shape.");

  const TopoDS_Face* face = get_face_(screen_coords);
  if (!face)
    return Status::user_error("Click a face (selection filter is Face).");

  std::optional<gp_Ax3> frame;
  if (m_pick == Pick::Planar_face)
  {
    const std::optional<gp_Pln> pln = plane_from_face(*face);
    if (!pln)
      return Status::user_error("Selected face is not planar.");

    const gp_Pnt origin = planar_face_origin_(*face, *pln);
    frame               = gp_Ax3(origin, pln->Axis().Direction(), pln->XAxis().Direction());
  }
  else
  {
    const std::optional<Cyl_face_info> cyl = cylinder_from_face(*face);
    if (!cyl)
      return Status::user_error("Selected face is not cylindrical.");

    const gp_Pnt origin = project_onto_axis_(cyl->axis, bbox_center_(m_target->Shape()));
    frame               = gp_Ax3(gp_Ax2(origin, cyl->axis.Direction()));
  }

  // Leave pick mode before the undo push so history stores Normal, not Shape_set_frame.
  const Shp_ptr target = m_target;
  clear_all(m_target);
  gui().set_mode(Mode::Normal);
  view().set_shape_frame(target, *frame);
  target->set_show_frame_axes(true);
  gui().show_message("Local frame updated.");
  return Status::ok();
}

void Shp_set_frame::cancel()
{
  clear_all(m_target);
  gui().set_mode(Mode::Normal);
}
