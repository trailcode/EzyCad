#include "shp_transform.h"

#include "utl_geom.h"

namespace
{
const Shp* first_solid_(const std::vector<Shp_ptr>& shps)
{
  for (const Shp_ptr& s : shps)
    if (!s.IsNull() && !s->is_group())
      return s.get();

  return nullptr;
}
} // namespace

Transform_axes transform_axes_for(const std::vector<Shp_ptr>& shps, Transform_space space)
{
  Transform_axes axes;
  axes.origin = gp_Pnt(0.0, 0.0, 0.0);
  axes.x      = gp_Dir(1.0, 0.0, 0.0);
  axes.y      = gp_Dir(0.0, 1.0, 0.0);
  axes.z      = gp_Dir(0.0, 0.0, 1.0);

  const Shp* s = first_solid_(shps);
  if (!s)
    return axes;

  if (space == Transform_space::Local)
  {
    const gp_Ax3& f = s->get_frame();
    axes.origin     = f.Location();
    axes.x          = f.XDirection();
    axes.y          = f.YDirection();
    axes.z          = f.Direction();
    return axes;
  }

  const gp_Pnt local_c = get_shape_bbox_center(s->Shape());
  axes.origin          = s->is_workbench() ? local_c.Transformed(s->placement_trsf()) : local_c;
  return axes;
}

gp_XYZ transform_axis_magnitudes(const Transform_axes&             axes,
                                 const gp_Vec&                     mouse_vec,
                                 bool                              constr_x,
                                 bool                              constr_y,
                                 bool                              constr_z,
                                 const std::optional<double>& override_x,
                                 const std::optional<double>& override_y,
                                 const std::optional<double>& override_z)
{
  const bool no_constr = !constr_x && !constr_y && !constr_z;
  auto       mag       = [&](bool constr, const gp_Dir& dir, const std::optional<double>& ov) -> double
  {
    if (!no_constr && !constr)
      return 0.0;

    return ov.has_value() ? *ov : mouse_vec.Dot(gp_Vec(dir));
  };

  return gp_XYZ(mag(constr_x, axes.x, override_x), mag(constr_y, axes.y, override_y), mag(constr_z, axes.z, override_z));
}

gp_Vec transform_translation(const Transform_axes& axes, const gp_XYZ& magnitudes)
{
  return gp_Vec(axes.x) * magnitudes.X() + gp_Vec(axes.y) * magnitudes.Y() + gp_Vec(axes.z) * magnitudes.Z();
}

gp_Vec transform_translation(const Transform_axes&             axes,
                             const gp_Vec&                     mouse_vec,
                             bool                              constr_x,
                             bool                              constr_y,
                             bool                              constr_z,
                             const std::optional<double>& override_x,
                             const std::optional<double>& override_y,
                             const std::optional<double>& override_z)
{
  return transform_translation(
      axes, transform_axis_magnitudes(axes, mouse_vec, constr_x, constr_y, constr_z, override_x, override_y, override_z));
}
