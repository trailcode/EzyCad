#pragma once

#include "shp.h"

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_XYZ.hxx>
#include <optional>
#include <vector>

/// Shared by Move / Rotate / Scale. Local = first selected solid's frame.
enum class Transform_space
{
  Local = 0,
  World = 1
};

struct Transform_axes
{
  gp_Pnt origin;
  gp_Dir x;
  gp_Dir y;
  gp_Dir z;
};

/// First non-group solid's local frame, or world XYZ at that solid's bbox center.
Transform_axes transform_axes_for(const std::vector<Shp_ptr>& shps, Transform_space space);

/// Distances along the current X/Y/Z axes (world or local), with optional locks.
gp_XYZ transform_axis_magnitudes(const Transform_axes&             axes,
                                 const gp_Vec&                     mouse_vec,
                                 bool                              constr_x,
                                 bool                              constr_y,
                                 bool                              constr_z,
                                 const std::optional<double>& override_x,
                                 const std::optional<double>& override_y,
                                 const std::optional<double>& override_z);

gp_Vec transform_translation(const Transform_axes& axes, const gp_XYZ& magnitudes);

/// View-plane mouse vector expressed on the current axes, with optional locks.
gp_Vec transform_translation(const Transform_axes&             axes,
                             const gp_Vec&                     mouse_vec,
                             bool                              constr_x,
                             bool                              constr_y,
                             bool                              constr_z,
                             const std::optional<double>& override_x,
                             const std::optional<double>& override_y,
                             const std::optional<double>& override_z);
