#pragma once

#include "shp_operation.h"

enum class Rotation_axis
{
  View_to_object,  // Rotate around view axis through object center
  X_axis,          // Rotate around global X axis
  Y_axis,          // Rotate around global Y axis
  Z_axis           // Rotate around global Z axis
};

class Shp_rotate : private Shp_operation_base
{
 public:
  Shp_rotate(Occt_view& view);

  [[nodiscard]] Status rotate_selected(const ScreenCoords& screen_coords);
  [[nodiscard]] Status show_angle_edit(const ScreenCoords& screen_coords);
  void                 finalize();
  void                 cancel();

  void          set_rotation_axis(Rotation_axis axis);
  Rotation_axis get_rotation_axis() const
  {
    return m_rotation_axis;
  }

 private:
  [[nodiscard]] Status ensure_start_state_();
  void                 preview_rotate_();
  void                 post_opts_();
  void                 update_rotation_axis_();
  void                 update_rotation_center_();

  std::optional<gp_Pln> m_rotate_pln;
  std::optional<gp_Pnt> m_initial_mouse_pos;
  std::optional<gp_Pnt> m_center;
  double                m_angle {0};
  Rotation_axis         m_rotation_axis {Rotation_axis::View_to_object};
  AIS_Shape_ptr         m_rotation_axis_vis;
  AIS_Shape_ptr         m_rotation_center_vis;
};